#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/atomic.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/suspend.h>
#include <linux/notifier.h>	//for pm notifier
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/mm.h>
#include "mtu3.h"

/*==============================================================================
                         EDIT HISTORY FOR MODULE
This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.
when        who         what, where, why
--------    ---         --------------------------------------------------------
03/17/2021  joe.huang  init ql power drv
05/07/2021  joe.huang  Added PM notification mechanism to tell the application to do the appropriate action
05/28/2021  joe.huang  Add DTR_WAKE pin
==============================================================================*/

enum {PIN_DTR_WAKE, PIN_WAKEUP_IN, PIN_SLEEP_IND, PIN_MAX};	//It's in the same order as DTS
enum {SLEEP_IND_RESUME = 0, SLEEP_IND_SUSPEND = 1};
enum {WAKEUP_IN_RESUME = 0, WAKEUP_IN_SUSPEND = 1};
enum {DTR_WAKE_RESUME = 0, DTR_WAKE_SUSPEND = 1};

// ssize_t switch_usb_power(struct ssusb_mtk *ssusb,const char *buf, size_t count);
char __iomem *ql_power_usb_base;

// #define POWER_NOTIFICTION_SYSFS

//Control the following array for "All Functional Masking" mechanism
#define wakeup_in_str {"wakeup_in\0"}
#define sleep_ind_str {"sleep_ind\0"}
//It's in the same order as enum
const char g_pin_name[PIN_MAX][20] =
{
	wakeup_in_str,
	sleep_ind_str,
};

typedef void (*dealy_func)(struct work_struct *work);

/*
Hibernation event handling for the kernel
*/
struct power_data_kernel
{
	bool is_have_irq:1;  //this gpio is have irq
	bool is_have_wlock:1; //this gpio is have wlock
	bool is_gpio_output:1;
	int gpio;
	int init_gpio_level;	//is output gpio init status
	int suspend_level;		//irq gpio suspend level
	int irq;
	char *lock_str;
	struct wakeup_source *wlock;
	struct delayed_work queue_work;
	struct delayed_work sysfs_notify_work;
	atomic_t notify_state;	//notify wakeup_src state
	dealy_func dy_func;		//wakeup delay func
	dealy_func sysfs_notify_func;	//sysfs notify delay func
	struct platform_device *pdev;
};

/*
Hibernation event handling for the application
*/
struct power_data_app
{
	struct device *dev;
	atomic_t notify_state;	//pm notify state
	struct notifier_block *pm_notifier;
	struct delayed_work queue_work;	//sync usb power delay func
	dealy_func ql_power_usb_func;
	int usb_cmode;
};

static struct power_data_kernel gPowerKernelData[PIN_MAX];


static void __wakeup_src_delayed_func(int pin_nr);
static irqreturn_t wakeup_src_irq(int irq, void *param);

#ifdef POWER_NOTIFICTION_SYSFS
static struct power_data_app gPowerDataApp;
static int ql_power_pm_notif(struct notifier_block *b, unsigned long v, void *d)
{
	switch (v) {
	case PM_SUSPEND_PREPARE:
		pr_info("[ql_power_pm_notif] suspending ql_power_pm_notif\n");	//Notification application release necessity power
		atomic_set(&gPowerDataApp.notify_state, 0);
		sysfs_notify(&gPowerDataApp.dev->kobj, NULL, "ql_power_notifier");	//sync application block code path
		mdelay(100);
		return 0;

	case PM_POST_SUSPEND:
		pr_info("[ql_power_pm_notif] resuming ql_power_pm_notif\n");	//Notification application recovery necessity power
		atomic_set(&gPowerDataApp.notify_state, 1);
		sysfs_notify(&gPowerDataApp.dev->kobj, NULL, "ql_power_notifier");	//sync application block code path
		return 0;
	default:
		return 0;
	}
}

static struct notifier_block ql_power_pm_notif_block = {
	.notifier_call = ql_power_pm_notif,
};

static ssize_t ql_power_notifier_show(struct device *dev, struct device_attribute *attr, char *buf) {
	if (!strcmp(attr->attr.name, "ql_power_notifier")) {
		return sprintf(buf, "%d\n", !!atomic_read(&gPowerDataApp.notify_state));
	} else {
		return -1;
	}
}
static DEVICE_ATTR(ql_power_notifier, 0444, ql_power_notifier_show, NULL);
#endif

static void wakeup_in_delayed_func(struct work_struct *work)
{
	__wakeup_src_delayed_func(PIN_WAKEUP_IN);
}

static void dtr_wake_delayed_func(struct work_struct *work)
{
	__wakeup_src_delayed_func(PIN_DTR_WAKE);
}

static void __wakeup_src_delayed_func(int pin_nr)
{
	bool is_schedule_dw = true;
	if ((!!gpio_get_value(gPowerKernelData[pin_nr].gpio)) == gPowerKernelData[pin_nr].suspend_level)
	{
		__pm_relax(gPowerKernelData[pin_nr].wlock);
	}
	else
	{
		__pm_stay_awake(gPowerKernelData[pin_nr].wlock);
	}

	//if wakeup source has been activated do not schedule delayed work  add by joe.huang 2020.12.28
	if(gPowerKernelData[pin_nr].wlock->active == 1)
		is_schedule_dw = false;

	pr_info("[ql_power] wakeup_src_irq: gpio(%d) = %d\n", gPowerKernelData[pin_nr].gpio, gpio_get_value(gPowerKernelData[pin_nr].gpio));

	pr_info("---dump wlock_wakeup: active:%d, [active_count]:%d, event_count:%d, relax_count:%d, [wakeup_count]:%d", \
		gPowerKernelData[pin_nr].wlock->active, gPowerKernelData[pin_nr].wlock->active_count, gPowerKernelData[pin_nr].wlock->event_count, \
		gPowerKernelData[pin_nr].wlock->relax_count, gPowerKernelData[pin_nr].wlock->wakeup_count);

	if(is_schedule_dw == true)
		schedule_delayed_work(&gPowerKernelData[pin_nr].queue_work, msecs_to_jiffies(500));
}

static irqreturn_t wakeup_src_irq(int irq, void *param)
{
	struct power_data_kernel *data = (struct power_data_kernel *)param;
	schedule_delayed_work(&data->queue_work, msecs_to_jiffies(500));

	return IRQ_HANDLED;
}

static int ql_pwr_gpio_init(struct power_data_kernel *data, const char *lock_str)
{
	int ret;
	int gpio;

	if (data == NULL){
		return -EINVAL;
	}

	gpio = data->gpio;

	if (gpio < 0){
		pr_err("[ql_power] %s : err gpio=%d\n", __func__, gpio);
		return -ENODEV;
	}

	ret = gpio_request(gpio, lock_str);

	if (ret < 0){
		pr_err("[ql_power] %s: gpio_request gpio=%d, err=%d\n", __func__, gpio, ret);
		return ret;
	}
	if(!data->is_gpio_output == true){
		ret = gpio_direction_input(gpio);
	}
	else{
		ret = gpio_direction_output(gpio, data->init_gpio_level);
	}
	if (ret < 0){
		pr_err("[ql_power] %s: gpio_direction gpio=%d, err=%d\n", __func__, gpio, ret);
		goto err;
	}
	if(data->is_have_irq == true)
	{
		data->irq = gpio_to_irq(gpio);
		if (data->irq < 0){
			pr_err("[ql_power] %s: gpio_to_irq gpio=%d, err=%d\n", __func__, gpio, data->irq);
			goto err;
		}
	}
	if(data->is_have_wlock == true)
	{
		if ((!!gpio_get_value(data->gpio)) == data->suspend_level){
			pr_info("[ql_power] %s.%d gpio:%d WAKEUP IN SUSPEND\n",__func__, __LINE__, data->gpio);
			__pm_relax(data->wlock);
		}
		else{
			pr_info("[ql_power] %s.%d gpio:%d WAKEUP IN RESUME\n",__func__, __LINE__, data->gpio);
			__pm_stay_awake(data->wlock);
		}
		if(data->dy_func){
			INIT_DELAYED_WORK(&data->queue_work, data->dy_func);
		}
		if(data->sysfs_notify_func){
			atomic_set(&data->notify_state, 0);
			INIT_DELAYED_WORK(&data->sysfs_notify_work, data->sysfs_notify_func);
		}
	}
	if(data->is_have_irq == true)
	{
		ret = request_any_context_irq(data->irq, wakeup_src_irq, \
							IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, lock_str, data);
		if(ret < 0){
			pr_err("[ql_power] %s.%d request wakeup_src_irq failed\n",__func__, __LINE__);
			goto err;
		}
		enable_irq_wake(data->irq);
	}
	pr_info("[ql_power] %s.%d request gpio:%d success\n",__func__, __LINE__, data->gpio);
	return 0;
err:
	gpio_free(gpio);

	return ret;
}

static int power_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (gPowerKernelData[PIN_WAKEUP_IN].gpio > 0)
	{
		if ((!!gpio_get_value(gPowerKernelData[PIN_WAKEUP_IN].gpio)) != WAKEUP_IN_SUSPEND){
			dev_err(&pdev->dev, "[ql_power] abort pm suspend, (wakeup_in value %d)\n", gpio_get_value(gPowerKernelData[PIN_WAKEUP_IN].gpio));
			return -EBUSY;
		}
		enable_irq_wake(gPowerKernelData[PIN_WAKEUP_IN].irq);
		pr_info("[ql_power] %s.%d wake_up_in suspending\n",__func__, __LINE__);
	}

	if (gPowerKernelData[PIN_SLEEP_IND].gpio > 0){
		gpio_set_value(gPowerKernelData[PIN_SLEEP_IND].gpio, SLEEP_IND_SUSPEND);
	}
	pr_info("[ql_power] %s.%d suspending\n",__func__, __LINE__);
	return 0;
}

static int power_resume(struct platform_device *pdev)
{
	if (gPowerKernelData[PIN_SLEEP_IND].gpio > 0){
		gpio_set_value(gPowerKernelData[PIN_SLEEP_IND].gpio, SLEEP_IND_RESUME);
	}

	dev_info(&pdev->dev, "[ql_power] %s.%d resumeing\n",__func__, __LINE__);
	return 0;
}

static void ql_pwr_gpio_destroy(struct power_data_kernel *data)
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
		__pm_relax(data->wlock);
		wakeup_source_unregister(data->wlock);
	}
	gpio_free(data->gpio);
}

#ifdef POWER_NOTIFICTION_SYSFS
static void power_notifier_sysfs_notify_init(struct device *dev)
{
	device_create_file(dev, &dev_attr_ql_power_notifier);
	atomic_set(&gPowerDataApp.notify_state, 1);
	gPowerDataApp.pm_notifier = &ql_power_pm_notif_block;
	register_pm_notifier(gPowerDataApp.pm_notifier);
	gPowerDataApp.dev = dev;
}
#endif

static int power_probe(struct platform_device *pdev)
{
	struct device *dev = &(pdev->dev);
	struct device_node *np = dev->of_node;
	const char *name = NULL;
	struct pinctrl *power_pinctrl;
	struct pinctrl_state *kpd_default;
	int cnt;
	int i, j,ret;

	for (j=0; j<PIN_MAX; j++){
		gPowerKernelData[j].gpio = -1;
	}

	//first, get io resoure from dts
	cnt = of_gpio_count(np);
	for (i=0; i<cnt; i++)
	{
		if (of_property_read_string_index(np, "ql,gpio-names", i, &name) == 0)
		{
			for (j=0; j<PIN_MAX; j++)
			{
				dev_info(dev, "[ql_power] j:%d i:%d :%s :%s --",j, i, name, g_pin_name[j]);
				if (strcmp(name, g_pin_name[j]) == 0){
					gPowerKernelData[j].gpio = of_get_gpio(np, i);
					dev_info(dev, "[ql_power] j:%d i:%d %s :%s success gpio:%d\r\n",j, i, name, g_pin_name[j], gPowerKernelData[j].gpio);
					break;
				}
			}
		}
	}

	power_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(power_pinctrl)) {
		dev_info(dev, "Cannot find power_pinctrl!\n");

		return (int)PTR_ERR(power_pinctrl);
	}

	kpd_default = pinctrl_lookup_state(power_pinctrl, "default");
	if (IS_ERR(kpd_default)) {
		dev_info(dev, "Cannot find kpd_default!\n");

		return (int)PTR_ERR(kpd_default);
	}

	ret = pinctrl_select_state(power_pinctrl,
				kpd_default);
	if(ret){
		dev_info(dev, "Cannot pinctrl_select_state!\n");
		return -1;
	}

	//last, Initialize the corresponding state for each GPIO
	if (gPowerKernelData[PIN_WAKEUP_IN].gpio >= 0)
	{
		dev_info(dev, "[ql_power] debug info wake_up_in gpio init\n");
		gPowerKernelData[PIN_WAKEUP_IN].wlock = wakeup_source_register(dev, "ql_wakeup_in");
		gPowerKernelData[PIN_WAKEUP_IN].pdev = pdev;
		gPowerKernelData[PIN_WAKEUP_IN].dy_func = wakeup_in_delayed_func;
		gPowerKernelData[PIN_WAKEUP_IN].is_have_irq = true;
		gPowerKernelData[PIN_WAKEUP_IN].is_have_wlock = true;
		gPowerKernelData[PIN_WAKEUP_IN].is_gpio_output = false;
		gPowerKernelData[PIN_WAKEUP_IN].suspend_level = WAKEUP_IN_SUSPEND;
		ql_pwr_gpio_init(&gPowerKernelData[PIN_WAKEUP_IN], "ql_wakeup_in");
	}

	if (gPowerKernelData[PIN_DTR_WAKE].gpio >= 0)
	{
		dev_info(dev, "[ql_power] debug info dtr_wake gpio init\n");
		gPowerKernelData[PIN_DTR_WAKE].wlock = wakeup_source_register(dev, "ql_dtr_wake");
		gPowerKernelData[PIN_DTR_WAKE].pdev = pdev;
		gPowerKernelData[PIN_DTR_WAKE].dy_func = dtr_wake_delayed_func;
		gPowerKernelData[PIN_DTR_WAKE].is_have_irq = true;
		gPowerKernelData[PIN_DTR_WAKE].is_have_wlock = true;
		gPowerKernelData[PIN_DTR_WAKE].is_gpio_output = false;
		gPowerKernelData[PIN_DTR_WAKE].suspend_level = DTR_WAKE_SUSPEND;
		ql_pwr_gpio_init(&gPowerKernelData[PIN_DTR_WAKE], "ql_dtr_wake");
	}

	if (gPowerKernelData[PIN_SLEEP_IND].gpio >= 0)
	{
		dev_info(dev, "[ql_power] debug info pin_sleep_ind gpio init\n");
		gPowerKernelData[PIN_SLEEP_IND].is_have_irq = false;
		gPowerKernelData[PIN_SLEEP_IND].is_have_wlock = false;
		gPowerKernelData[PIN_SLEEP_IND].is_gpio_output = true;
		ql_pwr_gpio_init(&gPowerKernelData[PIN_SLEEP_IND], "sleep_ind");
	}

	//add power_notifier init by joe.huang 2021.05.07
#ifdef POWER_NOTIFICTION_SYSFS
	power_notifier_sysfs_notify_init(dev);
	dev_info(dev, "[ql_power] power_notifier worker init success\n");
#endif

	return 0;
}

static int power_remove(struct platform_device *pdev)
{
	// struct device *dev = &(pdev->dev);
	int i;
	for (i=0; i<PIN_MAX; i++)
	{
		if (gPowerKernelData[i].gpio > 0){
			ql_pwr_gpio_destroy(&gPowerKernelData[i]);
		}
	}
	return 0;
}

static const struct of_device_id power_match[] =
{
	{.compatible = "ql,quectel-power",},
	{},
};

static struct platform_driver ql_power_driver = {
	.probe		= power_probe,
	.remove		= power_remove,
	.suspend	= power_suspend,
	.resume		= power_resume,
	.driver		= {
		.name = "ql-power",
		.owner = THIS_MODULE,
		.of_match_table = power_match,
	},
};

module_platform_driver(ql_power_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("quectel power driver");
MODULE_VERSION("1.0");
