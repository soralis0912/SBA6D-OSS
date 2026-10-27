// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>

when        who         what, where, why
--------    ---         --------------------------------------------------------
03/19/2021  joe.huang  Add usb_iddig gpio sys_notifier

 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/stddef.h>
#ifdef CONFIG_QL_CNT_USB_IDDIG
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#endif

#include "mtu3.h"
#include "mtu3_dr.h"
#include "mtu3_debug.h"
#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#ifdef CONFIG_MTU3_MD_USB_EP0_CTRL
#include <mt-plat/mtk_ccci_common.h>
#include "mtu3_md_sync.h"

static char *aplog_usb_port = "0 0";
module_param(aplog_usb_port, charp, 0644);
MODULE_PARM_DESC (aplog_usb_port, "aplog_usb_port status");
static char *gnss_usb_port = "0 1";
module_param(gnss_usb_port, charp, 0644);
MODULE_PARM_DESC (gnss_usb_port, "gnss_usb_port status");
static char *meta_usb_port = "0 2";
module_param(meta_usb_port, charp, 0644);
MODULE_PARM_DESC (meta_usb_port, "meta_usb_port status");
static char dipcmode = 0;

enum {
	DIPC_UNCFG = 0,
	DIPC_PCIE_ADV,
	DIPC_PCIE_ONLY,
	DIPC_DUALIPC,
	DIPC_USB_ONLY,
	DIPC_PCIE_ADV_LINKOK = 11
};

static ssize_t dipc_mode_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int mode;
	if (kstrtoint(buf, 10, &mode))
		return -EINVAL;

	dev_info(dev, "%s - dipc_mode %d\n", __func__, mode);
	dipcmode = mode;

#ifndef CONFIG_MTU3_MD_USB_EP0_CTRL
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	struct generic_pm_domain *usb_pd;

	if (mode != DIPC_PCIE_ADV &&
			mode != DIPC_DUALIPC &&
			mode != DIPC_USB_ONLY) {
		ssusb_clks_disable(ssusb);
		usb_pd = pd_to_genpd(dev->pm_domain);
		usb_pd->flags = 0;
		pm_runtime_put_sync(ssusb->dev);
		pm_runtime_disable(ssusb->dev);
		dev_info(dev, "%s - disable USB CLK/MTCMOS\n", __func__);
	}
#endif
	return count;
}

static ssize_t dipc_mode_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", dipcmode);
}
static DEVICE_ATTR_RW(dipc_mode);

static struct attribute *ssusb_dipc_attrs[] = {
	&dev_attr_dipc_mode.attr,
	NULL
};

static const struct attribute_group ssusb_dipc_group = {
	.attrs = ssusb_dipc_attrs,
};

#endif  //end of CONFIG_MTU3_MD_USB_EP0_CTRL

//add by joe.huang 2021.03.19
#ifdef CONFIG_QL_CNT_USB_IDDIG
typedef void (*dealy_func)(struct work_struct *work);
struct usb_otg_data
{
	bool is_have_irq:1;  //this gpio is have irq
	bool is_have_wlock:1; //this gpio is have wlock
	bool is_gpio_output:1;
	bool is_have_suspend:1;	//this gpio is support suspend
	int gpio;
	int suspend_level;		//irq gpio suspend level
	int init_gpio_level;	//is output gpio init status
	int irq;
	char *lock_str;
	struct wakeup_source wlock;
	struct delayed_work queue_work;
	struct delayed_work sysfs_notify_work;
	atomic_t notify_state;	//notify wakeup_src state
	dealy_func dy_func;		//wakeup delay func
	dealy_func sysfs_notify_func;	//sysfs notify delay func
	struct device *dev;
};

#define iddig_str {"usb_iddig\0"}
#define drvbus_str {"usb_drvbus\0"}

#define iddig_str_pos 0
#define drvbus_str_pos 1
enum ql_otg_pin {
		PIN_IDDIG_IN = iddig_str_pos, \
	  	PIN_DRVBUS_IN = drvbus_str_pos, \
		PIN_MAX,\
	};
const char ext_pin_name[PIN_MAX][20] =
{
	iddig_str,
	drvbus_str,
};
static struct usb_otg_data g_usb_otg[PIN_MAX+1];
static void __sw_otg_src_delayed_func(int pin_nr);
static irqreturn_t sw_otg_src_irq(int irq, void *param);

static irqreturn_t sw_otg_src_irq(int irq, void *param)
{
	struct usb_otg_data *data = (struct usb_otg_data *)param;
	if(irq == g_usb_otg[PIN_IDDIG_IN].irq)
	{
		atomic_set(&g_usb_otg[PIN_IDDIG_IN].notify_state, 1);
	}
	schedule_delayed_work(&data->queue_work, msecs_to_jiffies(500));

	return IRQ_HANDLED;
}

static void iddig_delayed_func(struct work_struct *work)
{
	__sw_otg_src_delayed_func(PIN_IDDIG_IN);
}

static void __sw_otg_src_delayed_func(int pin_nr)
{
	bool is_schedule_dw = true;
	if(g_usb_otg[pin_nr].is_have_wlock == true)
	{
		if ((!!gpio_get_value(g_usb_otg[pin_nr].gpio)) == g_usb_otg[pin_nr].suspend_level)
		{
			__pm_relax(&g_usb_otg[pin_nr].wlock);

		}
		else
		{
			__pm_stay_awake(&g_usb_otg[pin_nr].wlock);
		}
		//if sw_otg source has been activated do not schedule delayed work  add by joe.huang 2020.12.28
		if(g_usb_otg[pin_nr].wlock.active == 1)
			is_schedule_dw = false;
	}
	else{
		is_schedule_dw = false;
	}

	pr_info("%s: gpio(%d) = %d\n", \
			__func__ , g_usb_otg[pin_nr].gpio, gpio_get_value(g_usb_otg[pin_nr].gpio));
	gpio_set_value(g_usb_otg[PIN_DRVBUS_IN].gpio, !gpio_get_value(g_usb_otg[pin_nr].gpio));
	if(g_usb_otg[PIN_IDDIG_IN].sysfs_notify_func){
		schedule_delayed_work(&g_usb_otg[pin_nr].sysfs_notify_work, 0);
	}
/*
	pr_info("---dump wlock_sw_otg: active:%d, [active_count]:%d, event_count:%d, relax_count:%d, [sw_otg_count]:%d", \
		g_usb_otg[pin_nr].wlock->active, g_usb_otg[pin_nr].wlock->active_count, g_usb_otg[pin_nr].wlock->event_count, \
		g_usb_otg[pin_nr].wlock->relax_count, g_usb_otg[pin_nr].wlock->sw_otg_count);
*/
	if(is_schedule_dw == true)
		schedule_delayed_work(&g_usb_otg[pin_nr].queue_work, msecs_to_jiffies(500));
}

static int otg_gpio_init(struct device *dev, enum ql_otg_pin pin, int *gpio)
{
	int ret = 0;
	const char *name = NULL;
	struct pinctrl *otg_pinctrl;
	struct pinctrl_state *kpd_default;
	struct device_node *np = dev->of_node;
	struct usb_otg_data *data = &g_usb_otg[pin];

	char *lock_str = data->lock_str;

	otg_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(otg_pinctrl)) {
		dev_err(dev, "Cannot find otg_pinctrl!\n");

		return (int)PTR_ERR(otg_pinctrl);
	}

	kpd_default = pinctrl_lookup_state(otg_pinctrl, "default");
	if (IS_ERR(kpd_default)) {
		dev_err(dev, "Cannot find ecall_state!\n");

		return (int)PTR_ERR(kpd_default);
	}

	ret = pinctrl_select_state(otg_pinctrl,
				kpd_default);
	if(ret){
		dev_err(dev, "Cannot pinctrl_select_state!\n");
		goto Err_pinctrl;
	}
	//get io resoure from dts
	if (of_property_read_string_index(np, "ql,gpio-names", pin, &name) == 0)
	{
		// dev_info(dev, "j:%d :%s :%s\r\n",0, name, ext_pin_name[pin]);
		if (strcmp(name, ext_pin_name[pin]) == 0){
			*gpio = of_get_gpio(np, pin);
			dev_info(dev, " %s get :%s gpio:%d \r\n", __func__, name, *gpio);
		}
		else{
			dev_info(dev, " %s Error get :%s gpio:%d \r\n", __func__, name, *gpio);
			ret = -EINVAL;
			goto Err_pinctrl;
		}
	}
	if (*gpio < 0){
		dev_err(data->dev, "%s : err gpio=%d\n", __func__, *gpio);
		goto Err_pinctrl;
	}
	ret = gpio_request(*gpio, lock_str);

	if (ret < 0){
		dev_err(data->dev, "%s: gpio_request gpio=%d, err=%d\n", __func__, *gpio, ret);
		goto Err_pinctrl;
	}
	if(!data->is_gpio_output == true){
		ret = gpio_direction_input(*gpio);
	}
	else{
		ret = gpio_direction_output(*gpio, data->init_gpio_level);
	}
	if (ret < 0){
		dev_err(data->dev, "%s: gpio_direction gpio=%d, err=%d\n", __func__, *gpio, ret);
		gpio_free(*gpio);
		goto Err_pinctrl;
	}

	dev_info(data->dev, "%s gpio=%d success\n", __func__, *gpio);
	return 0;
Err_pinctrl:
	return ret;
}

static void iddig_sysfs_notify_work(struct work_struct *work) {
    struct usb_otg_data *data = container_of(work, struct usb_otg_data, sysfs_notify_work.work);

    if (atomic_read(&data->notify_state)) {
        atomic_set(&data->notify_state, 0);
        sysfs_notify(&data->dev->kobj, NULL, "iddig_state");
    }
}

static ssize_t iddig_state_show(struct device *dev, struct device_attribute *attr, char *buf) {
	if (!strcmp(attr->attr.name, "iddig_state")) {
		return sprintf(buf, "%d\n", !!gpio_get_value(g_usb_otg[PIN_IDDIG_IN].gpio));
	} else {
		return 0;
	}
}
static DEVICE_ATTR(iddig_state, 0444, iddig_state_show, NULL);

static void iddig_init_sysfs_notify(struct device *dev)
{
	device_create_file(dev, &dev_attr_iddig_state);
	if(g_usb_otg[PIN_IDDIG_IN].sysfs_notify_func){
		dev_info(dev, "[quectel] INIT_DELAYED sysfs_notify_work");
		atomic_set(&g_usb_otg[PIN_IDDIG_IN].notify_state, 0);
		INIT_DELAYED_WORK(&g_usb_otg[PIN_IDDIG_IN].sysfs_notify_work, g_usb_otg[PIN_IDDIG_IN].sysfs_notify_func);
	}
}

static int iddig_init_gpio_irq(struct usb_otg_data *data, const char *lock_str)
{
	int ret;
	int gpio;

	if (data == NULL){
		return -EINVAL;
	}
	gpio = data->gpio;

	if(data->is_have_irq == true)
	{
		data->irq = gpio_to_irq(gpio);
		if (data->irq < 0){
			dev_err(data->dev, "%s: gpio_to_irq gpio=%d, err=%d\n", __func__, gpio, data->irq);
			goto err;
		}
	}
	if(data->is_have_wlock == true)
	{
		if ((!!gpio_get_value(data->gpio)) == data->suspend_level){
			dev_info(data->dev, "%s.%d gpio:%d WAKEUP IN SUSPEND\n",__func__, __LINE__, data->gpio);
			__pm_relax(&data->wlock);
		}
		else{
			dev_info(data->dev, "%s.%d gpio:%d WAKEUP IN RESUME\n",__func__, __LINE__, data->gpio);
			__pm_stay_awake(&data->wlock);
		}
	}
	if(data->dy_func){
		INIT_DELAYED_WORK(&data->queue_work, data->dy_func);
	}
	if(data->sysfs_notify_func){
		atomic_set(&data->notify_state, 0);
		INIT_DELAYED_WORK(&data->sysfs_notify_work, data->sysfs_notify_func);
	}
	if(data->is_have_irq == true)
	{
		ret = request_any_context_irq(data->irq, sw_otg_src_irq, \
							IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, lock_str, data);
		if(ret < 0){
			dev_err(data->dev, "%s.%d request sw_otg_src_irq failed\n",__func__, __LINE__);
			goto err_req_irq;
		}
		enable_irq_wake(data->irq);
	}
	dev_info(data->dev, "%s.%d request gpio:%d irq success\n",__func__, __LINE__, data->gpio);
	return 0;
err_req_irq:
err:
	gpio_free(gpio);
	return ret;
}

static int otg_irq_sysfs_notify_func(struct device *dev)
{
	int ret = 0;
	int gpio = 0;
	g_usb_otg[PIN_IDDIG_IN].dev = dev;
	g_usb_otg[PIN_IDDIG_IN].dy_func = iddig_delayed_func;
	g_usb_otg[PIN_IDDIG_IN].sysfs_notify_func = iddig_sysfs_notify_work;
	g_usb_otg[PIN_IDDIG_IN].is_have_irq = true;
	g_usb_otg[PIN_IDDIG_IN].lock_str = "usb_otg_iddig";
	iddig_init_sysfs_notify(g_usb_otg[PIN_IDDIG_IN].dev);

	ret = otg_gpio_init(g_usb_otg[PIN_IDDIG_IN].dev, PIN_IDDIG_IN, &gpio);
	if (ret) {
		goto err_otg_gpio_init;
	}
	g_usb_otg[PIN_IDDIG_IN].gpio = gpio;

	ret = iddig_init_gpio_irq(&g_usb_otg[PIN_IDDIG_IN], g_usb_otg[PIN_IDDIG_IN].lock_str);
	if (ret) {
		goto err_otg_gpio_init;
	}

	g_usb_otg[PIN_DRVBUS_IN].dev = dev;
	g_usb_otg[PIN_DRVBUS_IN].is_gpio_output = true;
	g_usb_otg[PIN_DRVBUS_IN].is_have_irq = false;
	g_usb_otg[PIN_DRVBUS_IN].lock_str = "usb_otg_drvbus";
	otg_gpio_init(g_usb_otg[PIN_DRVBUS_IN].dev, PIN_DRVBUS_IN, &gpio);
	g_usb_otg[PIN_DRVBUS_IN].gpio = gpio;

	return 0;
err_otg_gpio_init:
	if(g_usb_otg[PIN_IDDIG_IN].dy_func){
		cancel_delayed_work(&g_usb_otg[PIN_IDDIG_IN].queue_work);
	}
	if(g_usb_otg[PIN_IDDIG_IN].sysfs_notify_func){
		atomic_set(&g_usb_otg[PIN_IDDIG_IN].notify_state, 0);
		cancel_delayed_work(&g_usb_otg[PIN_IDDIG_IN].sysfs_notify_work);
	}
	return ret;
}

static void ql_usb_gpio_destroy(struct usb_otg_data *data)
{
	if(data->dy_func){
		cancel_delayed_work(&data->queue_work);
	}
	if(data->sysfs_notify_func){
		atomic_set(&data->notify_state, 0);
		cancel_delayed_work(&data->sysfs_notify_work);
	}
	if(data->is_have_irq == true){
		disable_irq_wake(data->irq);
		free_irq(data->irq, NULL);
	}
	if(data->is_have_wlock == true){
		__pm_relax(&data->wlock);
		wakeup_source_unregister(&data->wlock);
	}
	if(data->gpio > 0){
		gpio_free(data->gpio);
	}
}
#endif  //end of CONFIG_QL_CNT_USB_IDDIG

void ssusb_smc_request(struct ssusb_mtk *ssusb, int op, u32 para)
{
	struct arm_smccc_res res;
 	u32 retval0;

	arm_smccc_smc(MTK_SIP_USB_CONTROL,
		op, para, 0, 0, 0, 0, 0, &res);

	retval0 = res.a0;

	dev_info(ssusb->dev, "[%s]req op=%d para=%d ret0=%d\n",
				__FUNCTION__, op, para, retval0);

	switch (retval0) {
		case MTU3_SMC_OK:
			dev_info(ssusb->dev, "smc_request ok\n");
			break;
		case MTU3_SMC_VCORE_ENTER_ERR:
			dev_err(ssusb->dev, "VCORE_ENTER_ERR\n");
			break;
		case MTU3_SMC_VCORE_LEAVE_ERR:
			dev_err(ssusb->dev, "VCORE_LEAVE_ERR\n");
			break;
		case MTU3_SMC_VCORE_PARA_ERR:
			dev_err(ssusb->dev, "VCORE_PARA_ERR\n");
			break;
		case MTU3_SMC_CG_PARA_ERR:
			dev_err(ssusb->dev, "CG_PARA_ERR\n");
			break;
		default:
			dev_err(ssusb->dev, "unexpected ret0\n");
			break;
	}
}

void ssusb_set_txdeemph(struct ssusb_mtk *ssusb)
{
	u32 txdeemph;

	if (!ssusb->gen1_txdeemph)
		return;

	txdeemph = mtu3_readl(ssusb->mac_base, U3D_TXDEEMPH);
	txdeemph &= ~PIPE_TXDEEMPH_MASK;
	txdeemph |= PIPE_TXDEEMPH(0x1);
	mtu3_writel(ssusb->mac_base, U3D_TXDEEMPH, txdeemph);

	txdeemph = mtu3_readl(ssusb->mac_base, U3D_CP5_CP7_TXDEEMPH);
	txdeemph &= ~PIPE_CP5_CP7_TXDEEMPH_MASK;
	txdeemph |= PIPE_CP5_CP7_TXDEEMPH(0x1);
	mtu3_writel(ssusb->mac_base, U3D_CP5_CP7_TXDEEMPH, txdeemph);
}

void ssusb_set_force_vbus(struct ssusb_mtk *ssusb, bool vbus_on)
{
	u32 u2ctl;
	u32 misc;

	if (!ssusb->force_vbus)
		return;

	u2ctl = mtu3_readl(ssusb->ippc_base, SSUSB_U2_CTRL(0));
	misc = mtu3_readl(ssusb->mac_base, U3D_MISC_CTRL);
	if (vbus_on) {
		u2ctl &= ~SSUSB_U2_PORT_OTG_SEL;
		misc |= VBUS_FRC_EN | VBUS_ON;
	} else {
		u2ctl |= SSUSB_U2_PORT_OTG_SEL;
		misc &= ~(VBUS_FRC_EN | VBUS_ON);
	}
	mtu3_writel(ssusb->ippc_base, SSUSB_U2_CTRL(0), u2ctl);
	mtu3_writel(ssusb->mac_base, U3D_MISC_CTRL, misc);
}

/* u2-port0 should be powered on and enabled; */
int ssusb_check_clocks(struct ssusb_mtk *ssusb, u32 ex_clks)
{
	void __iomem *ibase = ssusb->ippc_base;
	u32 value, check_val;
	int ret;

	check_val = ex_clks | SSUSB_SYS125_RST_B_STS | SSUSB_SYSPLL_STABLE |
			SSUSB_REF_RST_B_STS;

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS1, value,
			(check_val == (value & check_val)), 100, 20000);
	if (ret) {
		dev_err(ssusb->dev, "clks of sts1 are not stable!\n");
		return ret;
	}

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS2, value,
			(value & SSUSB_U2_MAC_SYS_RST_B_STS), 100, 10000);
	if (ret) {
		dev_err(ssusb->dev, "mac2 clock is not stable\n");
		return ret;
	}

	return 0;
}

#ifndef CONFIG_MTU3_MD_USB_EP0_CTRL
static int ssusb_phy_init(struct ssusb_mtk *ssusb)
{
	int i;
	int ret;

	for (i = 0; i < ssusb->num_phys; i++) {
		ret = phy_init(ssusb->phys[i]);
		if (ret)
			goto exit_phy;
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(ssusb->phys[i - 1]);

	return ret;
}

static int ssusb_phy_exit(struct ssusb_mtk *ssusb)
{
	int i;

	for (i = 0; i < ssusb->num_phys; i++)
		phy_exit(ssusb->phys[i]);

	return 0;
}
#endif
int ssusb_phy_power_on(struct ssusb_mtk *ssusb)
{
	int i;
	int ret = 0;

	mutex_lock(&ssusb->phy_mutex);
	if (ssusb->power_status)
		goto power_on_phy;
	for (i = 0; i < ssusb->num_phys; i++) {
		ret = phy_power_on(ssusb->phys[i]);
		if (ret)
			goto power_off_phy;
	}
	ssusb->power_status = true;
	goto power_on_phy;

power_off_phy:
	for (; i > 0; i--)
		phy_power_off(ssusb->phys[i - 1]);
power_on_phy:
	mutex_unlock(&ssusb->phy_mutex);
	return ret;
}

void ssusb_phy_power_off(struct ssusb_mtk *ssusb)
{
	unsigned int i;

	mutex_lock(&ssusb->phy_mutex);
	if (!ssusb->power_status)
		goto power_off_phy;

	for (i = 0; i < ssusb->num_phys; i++)
		phy_power_off(ssusb->phys[i]);
	ssusb->power_status = false;
power_off_phy:
	mutex_unlock(&ssusb->phy_mutex);
}

void ssusb_phy_set_mode(struct ssusb_mtk *ssusb, enum phy_mode mode)
{
	unsigned int i;

	for (i = 0; i < ssusb->num_phys; i++)
		phy_set_mode(ssusb->phys[i], mode);
}

int ssusb_clks_enable(struct ssusb_mtk *ssusb)
{
	int ret;
	dev_info(ssusb->dev, "%s\n", __func__);

#ifdef CONFIG_MTU3_USB_LOW_POWER
	ssusb_smc_request(ssusb, MTU3_SMC_CG, 0);
#endif

	ret = clk_prepare_enable(ssusb->sys_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable sys_clk\n");
		goto sys_clk_err;
	}

	ret = clk_prepare_enable(ssusb->ref_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable ref_clk\n");
		goto ref_clk_err;
	}

	ret = clk_prepare_enable(ssusb->mcu_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable mcu_clk\n");
		goto mcu_clk_err;
	}

	ret = clk_prepare_enable(ssusb->dma_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable dma_clk\n");
		goto dma_clk_err;
	}

	return 0;

dma_clk_err:
	clk_disable_unprepare(ssusb->mcu_clk);
mcu_clk_err:
	clk_disable_unprepare(ssusb->ref_clk);
ref_clk_err:
	clk_disable_unprepare(ssusb->sys_clk);
sys_clk_err:
	return ret;
}

void ssusb_clks_disable(struct ssusb_mtk *ssusb)
{
	dev_info(ssusb->dev, "%s\n", __func__);
	clk_disable_unprepare(ssusb->dma_clk);
	clk_disable_unprepare(ssusb->mcu_clk);
	clk_disable_unprepare(ssusb->ref_clk);
	clk_disable_unprepare(ssusb->sys_clk);

#ifdef CONFIG_MTU3_USB_LOW_POWER
	ssusb_smc_request(ssusb, MTU3_SMC_CG, 1);
#endif
}

static int ssusb_rscs_init(struct ssusb_mtk *ssusb)
{
	int ret = 0;
#ifndef CONFIG_MTU3_MD_USB_EP0_CTRL

	if (ssusb->plat_type == PLAT_FPGA)
	        goto phy_init;

	ret = regulator_enable(ssusb->vusb33);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable vusb33\n");
		goto vusb33_err;
	}

	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

phy_init:
	ret = ssusb_phy_init(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to init phy\n");
		goto phy_init_err;
	}

	ret = ssusb_phy_power_on(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to power on phy\n");
		goto phy_err;
	}

	return 0;

phy_err:
	ssusb_phy_exit(ssusb);
phy_init_err:
	if (ssusb->plat_type == PLAT_FPGA)
		return ret;

	ssusb_clks_disable(ssusb);
clks_err:
	regulator_disable(ssusb->vusb33);
vusb33_err:
	return ret;
#else
   	return ret;
#endif

}

static void ssusb_rscs_exit(struct ssusb_mtk *ssusb)
{
#ifndef CONFIG_MTU3_MD_USB_EP0_CTRL
	ssusb_clks_disable(ssusb);
	regulator_disable(ssusb->vusb33);
	ssusb_phy_power_off(ssusb);
	ssusb_phy_exit(ssusb);
#endif
}

void ssusb_ip_sw_reset(struct ssusb_mtk *ssusb)
{
	/* reset whole ip (xhci & u3d) */
	mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
	udelay(1);
	mtu3_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);

	/*
	 * device ip may be powered on in firmware/BROM stage before entering
	 * kernel stage;
	 * power down device ip, otherwise ip-sleep will fail when working as
	 * host only mode
	 */
	if (ssusb->dr_mode == USB_DR_MODE_HOST)
		mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL2,
				SSUSB_IP_DEV_PDN);
}

static int get_ssusb_rscs(struct platform_device *pdev, struct ssusb_mtk *ssusb)
{
	struct device_node *node = pdev->dev.of_node;
	struct otg_switch_mtk *otg_sx = &ssusb->otg_switch;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int i;
	int ret;

	ret = of_property_read_u32(node, "plat_type", &ssusb->plat_type);
	if (!ret && ssusb->plat_type == PLAT_FPGA) {
		dev_info(ssusb->dev, "platform is fpga\n");

		of_property_read_u32(node, "fpga_phy",
			&ssusb->fpga_phy);

		dev_info(ssusb->dev, "fpga phy is %d\n", ssusb->fpga_phy);
		goto get_phy;
	}

	ssusb->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(ssusb->vusb33)) {
		dev_err(dev, "failed to get vusb33\n");
		return PTR_ERR(ssusb->vusb33);
	}

	ssusb->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(ssusb->sys_clk)) {
		dev_err(dev, "failed to get sys clock\n");
		return PTR_ERR(ssusb->sys_clk);
	}

	ssusb->ref_clk = devm_clk_get_optional(dev, "ref_ck");
	if (IS_ERR(ssusb->ref_clk))
		return PTR_ERR(ssusb->ref_clk);

	ssusb->mcu_clk = devm_clk_get_optional(dev, "mcu_ck");
	if (IS_ERR(ssusb->mcu_clk))
		return PTR_ERR(ssusb->mcu_clk);

	ssusb->dma_clk = devm_clk_get_optional(dev, "dma_ck");
	if (IS_ERR(ssusb->dma_clk))
		return PTR_ERR(ssusb->dma_clk);

get_phy:
	ssusb->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (ssusb->num_phys > 0) {
		ssusb->phys = devm_kcalloc(dev, ssusb->num_phys,
					sizeof(*ssusb->phys), GFP_KERNEL);
		if (!ssusb->phys)
			return -ENOMEM;
	} else {
		ssusb->num_phys = 0;
	}

	for (i = 0; i < ssusb->num_phys; i++) {
		ssusb->phys[i] = devm_of_phy_get_by_index(dev, node, i);
		if (IS_ERR(ssusb->phys[i])) {
			dev_err(dev, "failed to get phy-%d\n", i);
			return PTR_ERR(ssusb->phys[i]);
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ippc");
	ssusb->ippc_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(ssusb->ippc_base))
		return PTR_ERR(ssusb->ippc_base);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "spm");
	if(!res)
		dev_err(dev, "spm res null\n");
	else {
		ssusb->spm_base = devm_ioremap(dev,res->start, resource_size(res));
		if (IS_ERR(ssusb->spm_base))
			dev_err(dev, "failed to get spm_base\n");
	}
#ifdef CONFIG_MTU3_USB_LOW_POWER
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pericfg");
	if(!res)
		dev_err(dev, "pericfg res null\n");
	else {
		ssusb->pericfg_ao_base = devm_ioremap(dev,res->start, resource_size(res));
		if (IS_ERR(ssusb->pericfg_ao_base))
			dev_err(dev, "failed to get pericfg_ao\n");
	}
#endif
	ssusb->force_vbus = of_property_read_bool(node, "mediatek,force-vbus");
	ssusb->clk_mgr = of_property_read_bool(node, "mediatek,clk-mgr");

	ssusb->gen1_txdeemph =
		of_property_read_bool(node, "mediatek,gen1-txdeemph");

	ssusb->dr_mode = usb_get_dr_mode(dev);
	if (ssusb->dr_mode == USB_DR_MODE_UNKNOWN)
		ssusb->dr_mode = USB_DR_MODE_OTG;

	if (ssusb->dr_mode == USB_DR_MODE_PERIPHERAL)
		goto out;

	/* if host role is supported */
	ret = ssusb_wakeup_of_property_parse(ssusb, node);
	if (ret) {
		dev_err(dev, "failed to parse uwk property\n");
		return ret;
	}

	/* optional property, ignore the error if it does not exist */
	of_property_read_u32(node, "mediatek,u3p-dis-msk",
			     &ssusb->u3p_dis_msk);

	otg_sx->vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(otg_sx->vbus)) {
		dev_err(dev, "failed to get vbus\n");
		return PTR_ERR(otg_sx->vbus);
	}

	if (ssusb->dr_mode == USB_DR_MODE_HOST)
		goto out;

	/* if dual-role mode is supported */
	otg_sx->is_u3_drd = of_property_read_bool(node, "mediatek,usb3-drd");
	otg_sx->manual_drd_enabled =
		of_property_read_bool(node, "enable-manual-drd");
	otg_sx->role_sw_used = of_property_read_bool(node, "usb-role-switch");

	if (!otg_sx->role_sw_used && of_property_read_bool(node, "extcon")) {
		otg_sx->edev = extcon_get_edev_by_phandle(ssusb->dev, 0);
		if (IS_ERR(otg_sx->edev)) {
			dev_err(ssusb->dev, "couldn't get extcon device\n");
			return PTR_ERR(otg_sx->edev);
		}
	}

out:
	dev_info(dev, "dr_mode: %d, is_u3_dr: %d, u3p_dis_msk: %x, drd: %s\n",
		ssusb->dr_mode, otg_sx->is_u3_drd, ssusb->u3p_dis_msk,
		otg_sx->manual_drd_enabled ? "manual" : "auto");

	return 0;
}

#ifdef CONFIG_MTU3_MD_USB_EP0_CTRL

void mtu3_md_ep0_setup_packet_info(struct mtu3 *mtu, struct usb_ctrlrequest *setup_packet)
{
	dev_info(mtu->dev, "bmRequestType:0x%X, bRequest:0x%X, wValue:0x%X, wIndex:0x%X, wLength:0x%X\n",
			setup_packet->bRequestType,
			setup_packet->bRequest,
			setup_packet->wValue,
			setup_packet->wIndex,
			setup_packet->wLength);
}

enum usb_device_speed mtu3_md_usb_speed_sync(usb_speed_enum_e md_speed)
{
	switch (md_speed) {
		case USB_SPEED_USB11:
			return USB_SPEED_FULL;
		case USB_SPEED_USB20:
			return USB_SPEED_HIGH;
		case USB_SPEED_USB30:
			return USB_SPEED_SUPER;
		default:
			return USB_SPEED_UNKNOWN;
	}
}

unsigned mtu3_md_usb_ep0_maxpacket(usb_speed_enum_e md_speed)
{
	switch (md_speed) {
		case USB_SPEED_USB11:
			return 64;
		case USB_SPEED_USB20:
			return 64;
		case USB_SPEED_USB30:
			return 512;
		default:
			return 0;
	}
}

void mtu3_forward_to_driver_work(struct work_struct *data)
{
	struct mtu3 *mtu;
	struct mtu3_md_sync_data *md_sync_data;
	mtu = container_of(to_delayed_work(data),
		struct mtu3, forward_to_driver_work);
	md_sync_data = mtu->md_sync_data;

	if (!mtu->gadget_driver) {
		schedule_delayed_work(&mtu->forward_to_driver_work,
		msecs_to_jiffies(100));
		return;
	} else {
		cancel_delayed_work(&mtu->forward_to_driver_work);
	}

	md_sync_data->setup = devm_kzalloc(mtu->dev, sizeof(struct usb_ctrlrequest), GFP_KERNEL);
	md_sync_data->setup->bRequestType = 0x00;
	md_sync_data->setup->bRequest = 0x09;
	md_sync_data->setup->wValue = 0x1;
	md_sync_data->setup->wIndex = 0x0;
	md_sync_data->setup->wLength = 0x0;

	mtu->g.speed = mtu3_md_usb_speed_sync(md_sync_data->speed);
	mtu->g.ep0->maxpacket = mtu3_md_usb_ep0_maxpacket(md_sync_data->speed);
	forward_to_driver(mtu, md_sync_data->setup);
}

void mtu3_forward_ctrlrequest_to_driver_work(struct work_struct *data)
{
	struct mtu3 *mtu;
	struct mtu3_md_sync_data *md_sync_data;
	mtu = container_of(to_delayed_work(data),
		struct mtu3, forward_ctrlrequest_to_driver_work);
	md_sync_data = mtu->md_sync_data;

	if (!(mtu->gadget_driver && mtu->g.state == USB_STATE_CONFIGURED)) {
		schedule_delayed_work(&mtu->forward_ctrlrequest_to_driver_work,
				msecs_to_jiffies(200));
		return;
	} else {
		cancel_delayed_work(&mtu->forward_ctrlrequest_to_driver_work);
	}

	forward_to_driver(mtu, md_sync_data->ctrl_req);
}

void mtu3_msg_to_md(struct mtu3 *mtu,apusb_msg_id_e msg_id)
{
	int ret = 0;
	struct apusb_rsp_s mrsp;
	mrsp.msg_id = msg_id;
	mrsp.msg_length = 8;
	memset(mrsp.data, 0, CCCI_PAYLOAD_SIZE);
	ret = mtk_ccci_write_data(mtu->ccci_usb_index,
		(char *)&mrsp, mrsp.msg_length);

	if (ret < 0) {
		dev_err(mtu->dev, "%s write_data fail msg_id=%d !\n",
		__func__,msg_id);
	} else {
		dev_info(mtu->dev, "%s echo md success msg_id=%d\n",
		__func__,msg_id);
	}
}

int md_msg_hdlr(void *arg)
{
	struct ssusb_mtk *ssusb = arg;
	struct mtu3 *mtu = ssusb->u3d;
	struct mtu3_md_sync_data *md_sync_data = mtu->md_sync_data;
	struct usb_ctrlrequest *setup_packet;
	dual_usb_owner_msg_s_t *msg;
	u32 total_class_device_num;
	int read_count, class_dev_idx, intf_idx, ep_idx;
	char *buf;

	md_sync_data->usb_device_info = devm_kzalloc(ssusb->dev, sizeof(usb_device_info_s_t), GFP_KERNEL);
	buf = devm_kzalloc(ssusb->dev, CCCI_PAYLOAD_SIZE, GFP_KERNEL);

	dev_info(mtu->dev, "MD msg polling thread is running\n");
	while (!kthread_should_stop()) {
		read_count = mtk_ccci_read_data(mtu->ccci_usb_index, buf, CCCI_PAYLOAD_SIZE);
		if (read_count != -ENODEV) {
			dev_info(mtu->dev, "ccci read count:%d\n", read_count);
			msg = (dual_usb_owner_msg_s_t *) buf;
			switch (msg->msg_type) {
			case NOTIFY_EVT_ATTACHING:
				dev_info(mtu->dev, "NOTIFY_EVT_ATTACHING\n");
				if(mtu->ep_array == NULL)
				{
					mtu3_mem_alloc_set(mtu);
					mtu3_gadget_setup(mtu);
				}
				break;
			case NOTIFY_EVT_ATTACHED:
				dev_info(mtu->dev, "NOTIFY_EVT_ATTACHED\n");
				md_sync_data->intr_id_list = 0;
				md_sync_data->usb_device_info = (usb_device_info_s_t *) msg->msg_data;

				total_class_device_num = md_sync_data->usb_device_info->total_class_device_num;
				dev_info(mtu->dev, "total_class_device_num:%d\n", total_class_device_num);

				for (class_dev_idx = 0; class_dev_idx < total_class_device_num; class_dev_idx++) {
					class_device_info_s_t *class_device_info = &md_sync_data->usb_device_info->class_device_info[class_dev_idx];
					u32 class_device_id = class_device_info->class_device_id;
					usb_device_type_e class_device_type = class_device_info->class_device_type;
					u32 total_interface_num = class_device_info->total_interface_num;

					dev_info(mtu->dev, "class[%d] id:%d, type:%d, int_num:%d\n", class_dev_idx,
						class_device_id,
						class_device_type,
						total_interface_num);

					for (intf_idx = 0; intf_idx < total_interface_num; intf_idx++) {
						interface_info_s_t *interface_info = &class_device_info->if_info[intf_idx];
						u32 interface_id = interface_info->interface_id;
						usb_interface_type_e interface_type = interface_info->interface_type;
						int ep_num = 0;

						dev_info(mtu->dev, "class[%d]intf[%d] - intf_id:%d, intf_type:%d\n",
							class_dev_idx, intf_idx, interface_id, interface_type);
						switch (class_device_type) {
						case USB_CLASS_TYPE_CDC_ACM1:
							dev_info(mtu->dev, "ACM 1T1R\n");
							ep_num = 2;
							break;
						case USB_CLASS_TYPE_ADB:
							ep_num = 2;
							dev_info(mtu->dev, "ADB\n");
							break;
						case USB_CLASS_TYPE_MS:
							dev_info(mtu->dev, "MSC\n");
							ep_num = 2;
							break;
						default:
							dev_warn(mtu->dev, "Incorrect Class Device Type:%d\n", class_device_type);
							break;
						}

						for (ep_idx = 0; ep_idx < ep_num; ep_idx++) {
							endpoint_info_s_t *ep_info = &class_device_info->ep_info[ep_idx];
							u32 class_device_id = ep_info->class_device_id;
							u32 ep_address = ep_info->ep_address;
							usb_ep_type_e ep_type = ep_info->ep_type;
							u32 max_packetsize = ep_info->max_packetsize;
							dev_info(mtu->dev, "endp[%d] id:0x%X, addr:0x%X, type:0x%X, max_pkt_sz:%d\n", ep_idx,
								class_device_id,
								ep_address,
								ep_type,
								max_packetsize);

							switch (ep_address & USB_ENDPOINT_DIR_MASK) {
							case USB_DIR_IN:
								md_sync_data->tx_ep |= 1 << (ep_address & 0xf);
								break;

							case USB_DIR_OUT:
								md_sync_data->rx_ep |= 1 << ep_address;
								break;
							}
						}
						dev_info(mtu->dev, "tx_ep:0x%X, rx_ep:0x%X\n",
							md_sync_data->tx_ep, md_sync_data->rx_ep);
						md_sync_data->intr_id_list |= 1 << interface_id;
					}
				}
				schedule_delayed_work(&mtu->forward_to_driver_work,
				msecs_to_jiffies(100));
				if (total_class_device_num) {
					aplog_usb_port = "1 0";
					gnss_usb_port = "1 1";
					meta_usb_port = "1 2";
				} else {
					aplog_usb_port = "0 0";
					gnss_usb_port = "0 1";
					meta_usb_port = "0 2";
					dev_err(mtu->dev, "Incorrect MD configuration !!!!!!!\n");
				}
				pm_stay_awake(ssusb->dev);
				mtu3_intr_control(mtu, true);
				break;

			case NOTIFY_EVT_DETACHED:
				dev_info(mtu->dev, "NOTIFY_EVT_DETACHED\n");
				aplog_usb_port = "0 0";
				gnss_usb_port = "0 1";
				meta_usb_port = "0 2";
				mtu3_gadget_disconnect(mtu);
				mtu3_intr_control(mtu, false);
				mtu3_msg_to_md(mtu, APUSB_NOTIFY_EVT_DETACHED_DONE);

				pm_relax(ssusb->dev);
				break;

			case NOTIFY_EVT_SUSPEND:
				dev_info(mtu->dev, "NOTIFY_EVT_SUSPEND\n");

				mtu3_gadget_suspend(mtu);
				ssusb_smc_request(ssusb, MTU3_SMC_VCORE, 1);
				mtu3_msg_to_md(mtu, APUSB_NOTIFY_EVT_SUSPEND_DONE);

				pm_relax(ssusb->dev);
				break;

			case NOTIFY_EVT_RESUME:
				dev_info(mtu->dev, "NOTIFY_EVT_RESUME\n");
				pm_stay_awake(ssusb->dev);

				ssusb_smc_request(ssusb, MTU3_SMC_VCORE, 0);
				mtu3_gadget_resume(mtu);
				break;

			case NOTIFY_EVT_RESET:
				dev_info(mtu->dev, "NOTIFY_EVT_RESET\n");
				pm_stay_awake(ssusb->dev);
				md_sync_data->speed = *(usb_speed_enum_e *) msg->msg_data;
				dev_info(mtu->dev, "USB reset speed:%d\n", md_sync_data->speed);
				aplog_usb_port = "0 0";
				gnss_usb_port = "0 1";
				meta_usb_port = "0 2";
				ssusb_smc_request(ssusb, MTU3_SMC_VCORE, 0);
				mtu3_gadget_reset(mtu);

				mtu3_msg_to_md(mtu, APUSB_NOTIFY_EVT_RESET_DONE);
				break;

			case NOTIFY_EVT_CLASS_CTRL_REQ:
				dev_info(mtu->dev, "NOTIFY_EVT_CLASS_CTRL_REQ\n");

				setup_packet = (struct usb_ctrlrequest *) msg->msg_data;
				mtu3_md_ep0_setup_packet_info(mtu, setup_packet);

				md_sync_data->ctrl_req = devm_kzalloc(mtu->dev, sizeof(struct usb_ctrlrequest), GFP_KERNEL);
				md_sync_data->ctrl_req->bRequestType = setup_packet->bRequestType;
				md_sync_data->ctrl_req->bRequest = setup_packet->bRequest;
				md_sync_data->ctrl_req->wValue = setup_packet->wValue;
				md_sync_data->ctrl_req->wIndex = setup_packet->wIndex;
				md_sync_data->ctrl_req->wLength = setup_packet->wLength;
				schedule_delayed_work(&mtu->forward_ctrlrequest_to_driver_work,
						msecs_to_jiffies(200));

				break;

			case NOTIFY_EVT_FEATURE_SC:
				dev_info(mtu->dev, "NOTIFY_EVT_FEATURE_SC\n");
				setup_packet = (struct usb_ctrlrequest *) msg->msg_data;

				mtu3_md_ep0_setup_packet_info(mtu, setup_packet);

				mtu3_ep0_handle_setup(mtu, setup_packet);

				if ((setup_packet->bRequestType & USB_RECIP_MASK)
					== USB_RECIP_ENDPOINT) {
					if((setup_packet->bRequest == USB_REQ_CLEAR_FEATURE)
					||(setup_packet->bRequest ==USB_REQ_SET_FEATURE)){
						mtu3_msg_to_md(mtu, APUSB_RSP_STATUS_SUCCESS);
					}
				}
				break;

			case NOTIFY_EVT_CTRL_DATA:
				dev_info(mtu->dev, "NOTIFY_EVT_CTRL_DATA\n");

				md_sync_data->req->length = (unsigned) md_sync_data->ctrl_req->wLength;
				/* msg_length = msg header(8) size + msg data*/
				md_sync_data->req->actual = (unsigned) (msg->msg_length - 8);
				md_sync_data->req->buf = (void *) msg->msg_data;
				md_sync_data->req->status = 0;
				dev_info(mtu->dev, "req_length=%d req->actual=%d\n",
						    md_sync_data->req->length,
						    md_sync_data->req->actual);
				md_sync_data->req->complete(&(mtu->ep0)->ep, md_sync_data->req);
				break;

			case NOTIFY_FUNCTION_RESUME:
				dev_info(mtu->dev, "NOTIFY_FUNCTION_RESUME\n");
				break;

			default:
				dev_info(mtu->dev, "MD msg type:%d\n", msg->msg_type);
			}
		} else {
			dev_info(mtu->dev, "ccci read count:%d (MD not ready)\n", read_count);
			msleep(1000); //prevent polling log too much when not ready
		}
		msleep(1);
	}
	return 0;
}
#endif
static int mtu3_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct ssusb_mtk *ssusb;

	int ret = -ENOMEM;

	/* all elements are set to ZERO as default value */
	ssusb = devm_kzalloc(dev, sizeof(*ssusb), GFP_KERNEL);
	if (!ssusb)
		return -ENOMEM;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "No suitable DMA config available\n");
		return -ENOTSUPP;
	}

	platform_set_drvdata(pdev, ssusb);
	ssusb->dev = dev;

	ret = get_ssusb_rscs(pdev, ssusb);
	if (ret)
		return ret;

	ssusb_debugfs_create_root(ssusb);

	dev_info(dev, "%s - enbale USB MTCMOS\n", __func__);
	/* enable power domain */
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	device_enable_async_suspend(dev);
#ifdef CONFIG_QL_CNT_USB_IDDIG
	//add usb_iddig gpio init by joe.huang 2020.03.19
	ret = otg_irq_sysfs_notify_func(dev);
	if (ret) {
		dev_info(dev, "otg gpio worker init failed\n");
		// goto comm_init_err;
	}
	dev_info(dev, "[quectel]otg gpio worker init success\n");
#endif
	mutex_init(&ssusb->phy_mutex);
	mutex_lock(&ssusb->phy_mutex);
	ssusb->power_status = false;
	mutex_unlock(&ssusb->phy_mutex);

	ret = ssusb_rscs_init(ssusb);
	if (ret)
		goto comm_init_err;

	if (IS_ENABLED(CONFIG_USB_MTU3_HOST))
		ssusb->dr_mode = USB_DR_MODE_HOST;
	else if (IS_ENABLED(CONFIG_USB_MTU3_GADGET))
		ssusb->dr_mode = USB_DR_MODE_PERIPHERAL;

	ssusb_ip_sw_reset(ssusb);

	/* default as host */
	ssusb->is_host = !(ssusb->dr_mode == USB_DR_MODE_PERIPHERAL);

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ret = ssusb_gadget_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto comm_exit;
		}
		break;
	case USB_DR_MODE_HOST:
		ret = ssusb_host_init(ssusb, node);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto comm_exit;
		}
		break;
	case USB_DR_MODE_OTG:
		ret = ssusb_gadget_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto comm_exit;
		}

		ret = ssusb_host_init(ssusb, node);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto gadget_exit;
		}

		ret = ssusb_otg_switch_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize switch\n");
			goto host_exit;
		}
		break;
	default:
		dev_err(dev, "unsupported mode: %d\n", ssusb->dr_mode);
		ret = -EINVAL;
		goto comm_exit;
	}

#ifdef CONFIG_MTU3_MD_USB_EP0_CTRL
	ssusb->u3d->ccci_usb_index = mtk_ccci_open_port("ccci_usbfwd");
	if (ssusb->u3d->ccci_usb_index < 0) {
		dev_err(dev, "mtk_ccci_open_port FAIL (0x%X)!\n", ssusb->u3d->ccci_usb_index);
		goto comm_exit;
	}
	dev_dbg(dev, "ccci_usb_index:%d\n", ssusb->u3d->ccci_usb_index);
	ssusb->u3d->md_msg_polling_tsk = kthread_run(md_msg_hdlr, ssusb, "%s", "md_msg_hdlr");

	ret = sysfs_create_group(&ssusb->dev->kobj, &ssusb_dipc_group);
	if (ret)
		dev_info(ssusb->dev, "error creating sysfs attributes\n");

#endif
	return 0;

host_exit:
	ssusb_host_exit(ssusb);
gadget_exit:
	ssusb_gadget_exit(ssusb);
comm_exit:
	ssusb_rscs_exit(ssusb);
comm_init_err:
#ifdef CONFIG_QL_CNT_USB_IDDIG
	ql_usb_gpio_destroy(&g_usb_otg[PIN_IDDIG_IN]);
	ql_usb_gpio_destroy(&g_usb_otg[PIN_DRVBUS_IN]);
#endif
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	ssusb_debugfs_remove_root(ssusb);
	#ifdef CONFIG_MTU3_MD_USB_EP0_CTRL
	sysfs_remove_group(&ssusb->dev->kobj, &ssusb_dipc_group);
	#endif

	return ret;
}

static int mtu3_remove(struct platform_device *pdev)
{
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ssusb_gadget_exit(ssusb);
		break;
	case USB_DR_MODE_HOST:
		ssusb_host_exit(ssusb);
		break;
	case USB_DR_MODE_OTG:
		ssusb_otg_switch_exit(ssusb);
		ssusb_gadget_exit(ssusb);
		ssusb_host_exit(ssusb);
		break;
	default:
		return -EINVAL;
	}

	ssusb_rscs_exit(ssusb);
#ifdef CONFIG_QL_CNT_USB_IDDIG
	ql_usb_gpio_destroy(&g_usb_otg[PIN_IDDIG_IN]);
	ql_usb_gpio_destroy(&g_usb_otg[PIN_DRVBUS_IN]);
#endif
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	ssusb_debugfs_remove_root(ssusb);
	#ifdef CONFIG_MTU3_MD_USB_EP0_CTRL
	sysfs_remove_group(&ssusb->dev->kobj, &ssusb_dipc_group);
	#endif

	return 0;
}

/*
 * when support dual-role mode, we reject suspend when
 * it works as device mode;
 */
static int __maybe_unused mtu3_suspend(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);

#ifdef CONFIG_MTU3_MD_USB_EP0_CTRL
	if (dipcmode == DIPC_PCIE_ADV ||
			dipcmode == DIPC_DUALIPC ||
			dipcmode == DIPC_USB_ONLY) {
		dev_info(dev, "%s - NO suspend in dipcmode:%d\n", __func__, dipcmode);
		return 0;
	}
#else
	dev_info(dev, "%s\n", __func__);
#endif

	/* REVISIT: disconnect it for only device mode? */
	if (!ssusb->is_host)
	{
#ifdef CONFIG_MTU3_USB_LOW_POWER
		mtu3_lowpower_set(ssusb->u3d, true);
#endif
		return 0;
	}
	ssusb_host_disable(ssusb, true);
	ssusb_phy_power_off(ssusb);
	ssusb_clks_disable(ssusb);
	ssusb_wakeup_set(ssusb, true);

	return 0;
}

static int __maybe_unused mtu3_resume(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	int ret;

#ifdef CONFIG_MTU3_MD_USB_EP0_CTRL
	if ((mtu3_readl(ssusb->spm_base, SPM_WAKEUP_SRC) & USB_POWERDWN_EVENT))
	{
		pm_stay_awake(dev);
		dev_info(dev, "wakelock by USB\n");
	}

		if (dipcmode == DIPC_PCIE_ADV ||
				dipcmode == DIPC_DUALIPC ||
				dipcmode == DIPC_USB_ONLY) {
			dev_info(dev, "%s - NO resume in dipcmode:%d\n", __func__, dipcmode);
			return 0;
		}
#else
	dev_info(dev, "%s\n", __func__);
#endif

	if (!ssusb->is_host)
	{
#ifdef CONFIG_MTU3_USB_LOW_POWER
		mtu3_lowpower_set(ssusb->u3d, false);
#endif
		return 0;
	}
	ssusb_wakeup_set(ssusb, false);
	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

	ret = ssusb_phy_power_on(ssusb);
	if (ret)
		goto phy_err;

	ssusb_host_enable(ssusb);

	return 0;

phy_err:
	ssusb_clks_disable(ssusb);
clks_err:
	return ret;
}

static const struct dev_pm_ops mtu3_pm_ops = {
	.suspend_noirq = mtu3_suspend,
	.resume_noirq = mtu3_resume,
};

#define DEV_PM_OPS (IS_ENABLED(CONFIG_PM) ? &mtu3_pm_ops : NULL)

#ifdef CONFIG_OF

static const struct of_device_id mtu3_of_match[] = {
	{.compatible = "mediatek,mt8173-mtu3",},
	{.compatible = "mediatek,mtu3",},
	{},
};

MODULE_DEVICE_TABLE(of, mtu3_of_match);

#endif

static struct platform_driver mtu3_driver = {
	.probe = mtu3_probe,
	.remove = mtu3_remove,
	.driver = {
		.name = MTU3_DRIVER_NAME,
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(mtu3_of_match),
	},
};
module_platform_driver(mtu3_driver);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek USB3 DRD Controller Driver");
