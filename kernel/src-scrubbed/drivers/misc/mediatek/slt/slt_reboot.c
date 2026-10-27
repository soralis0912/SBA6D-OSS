#include <linux/kernel.h>
#include <linux/interrupt.h>

#include <linux/module.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gfp.h>
#include <asm/io.h>
#include <asm/memory.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <asm/pgtable.h>
#include <linux/reboot.h>
//#include <disp_assert_layer.h>
#include <mtk_slt.h>

#define DRIVER_ATTR(_name, _mode, _show, _store) \
        struct driver_attribute driver_attr_##_name = \
        __ATTR(_name, _mode, _show, _store)

/* set MTK_EVB to 1 to disable printing messages to display */
#define MTK_EVB 1

//extern int DAL_Clean(void);
//extern int DAL_Printf(const char *fmt, ...);
//extern int LCD_LayerEnable(int id, bool enable);

#ifdef MTK_EVB
#define mtk_thermal_get_temp(...) 0
#define SLT_DISP_PRINT(...)
#define DAL_SetScreenColor(...)
#else
#define SLT_DISP_PRINT(...) \
	do { if (!MTK_EVB) DAL_Printf(__VA_ARGS__); } while (0)
#endif

void slt_print_bin(int mem, int stressapp, int reboot)
{
	int bin = 8;
	char buf[1024];
	sprintf(buf, "[STRESS SLT] MEMTEST %s, STRESSAPP %s, REBOOT TEST %s\n",
		mem ? "PASS" : "FAIL",
		stressapp ? "PASS" : "FAIL",
		reboot ? "PASS" : "FAIL");

	pr_emerg("%s", buf);


	if(mem && stressapp && reboot)
	{
		DAL_SetScreenColor(DAL_COLOR_YELLOW);
		bin = 1;
	}
	else if (!stressapp)
	{
		DAL_SetScreenColor(DAL_COLOR_BLUE);
		bin = 5;
	}
	else if (!mem)
	{
		DAL_SetScreenColor(DAL_COLOR_BLUE);
		bin = 6;
	}
	else if (!reboot)
	{
		DAL_SetScreenColor(DAL_COLOR_BLUE);
		bin = 7;
	}

	SLT_DISP_PRINT("%s", buf);

	if (!MTK_EVB)
		mdelay(3000);

	pr_emerg("[STRESS SLT] SLT_BIN%d\n", bin);

}

void slt_check_status(void)
{
#ifdef CONFIG_THERMAL
	pr_emerg("[STRESS SLT] temp: %d\n", mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU)/1000);
#endif

	if(slt_get_reboot_test_enabled()){
		slt_ram_mem_pass_sync();
		slt_ram_stressapp_pass_sync();
		switch(slt_get_last_wdt_status()){
			case 0:
			case 2: /* normal reboot */
				pr_emerg("[STRESS SLT] last wdt status: NORMAL (wdt_status = %d)\n", slt_get_last_wdt_status());
				pr_emerg("[STRESS SLT] reboot test: still %lld times to go\n", slt_get_reboot_count());
				SLT_DISP_PRINT("[GEM STRESS SLT] REBOOT TEST: still %lld times to go\n", slt_get_reboot_count());
				if(slt_get_reboot_count() > 0){
					/* restart */
					slt_set_reboot_count((slt_get_reboot_count()-1));
					slt_set_reboot_test_enabled(1);
					dsb(sy);
					pr_emerg("[STRESS SLT] reboot test: reboot system..\n");
					SLT_DISP_PRINT("[GEM STRESS SLT] REBOOT TEST: reboot system..\n");
					if (!MTK_EVB)
						mdelay(3000);
					kernel_restart(NULL);
				}else{
					/* end of test */
					pr_emerg("[STRESS SLT] reboot test: PASSED\n");
					slt_set_reboot_test_enabled(0);
					slt_set_reboot_count(0);
					DAL_SetScreenColor(DAL_COLOR_YELLOW);
					SLT_DISP_PRINT("[GEM STRESS SLT] REBOOT TEST: PASSED\n");
					dsb(sy);
					slt_print_bin(slt_get_mem_pass(), slt_get_stressapp_pass(), 1);
				}
				break;
			default: /* reboot test failed */
				pr_emerg("[STRESS SLT] reboot test: FAILED (wdt_status = %d)\n", slt_get_last_wdt_status());
				slt_set_reboot_test_enabled(0);
				slt_set_reboot_count(0);
				DAL_SetScreenColor(DAL_COLOR_RED | DAL_COLOR_BLUE);
				SLT_DISP_PRINT("[GEM STRESS SLT] REBOOT TEST FAILED\n");
				dsb(sy);
				slt_print_bin(slt_get_mem_pass(), slt_get_stressapp_pass(), 0);
				break;
		}
	}

	mdelay(1000);
}


/* stress SLT: testing driver */
static struct platform_driver slt_reboot_drv =
{
	.driver = {
		.name = "slt_reboot",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
};

static ssize_t reboot_test_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "usage: echo $reboot_loop_num > /sys/bus/platform/drivers/slt_reboot/reboot_test\n");
}

static ssize_t reboot_test_store(struct device_driver *driver, const char *buf, size_t count)
{
	int ret, reboot_num;

	if ((ret = sscanf(buf, "%d", &reboot_num)) != 1){
		pr_emerg("bad argument!!\n");
		pr_emerg("usage: echo $reboot_loop_num > /sys/bus/platform/drivers/slt_reboot/reboot_test\n");
		return -EINVAL;
	}

#ifdef CONFIG_THERMAL
	pr_emerg("[STRESS SLT] temp: %d\n", mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU)/1000);
#endif
	if(slt_get_reboot_test_enabled() != 1){
		pr_emerg("[STRESS SLT] reboot test: START (reboot looping for %d times)\n", reboot_num);
		SLT_DISP_PRINT("[GEM STRESS SLT] REBOOT TEST: START (reboot looping for %d times)\n", reboot_num);
		slt_set_reboot_test_enabled(1);
		slt_set_reboot_count(reboot_num);
		dsb(sy);
		pr_emerg("[STRESS SLT] reboot test: reboot system..\n");
		SLT_DISP_PRINT("[GEM STRESS SLT] REBOOT TEST: reboot system..\n");
		if (!MTK_EVB)
			mdelay(3000);
		kernel_restart(NULL);
	}

	return count;
}

static ssize_t report_store(struct device_driver *driver, const char *buf, size_t count)
{
	char *memtester_str = "memtester:";
	char *memtester_pass_str = "memtester: PASSED";
	char *stressapp_fail_str = "stressapptest: Status: FAILED";
	char *stressapp_pass_str = "stressapptest: Status: PASS";
	char *bootanim_end_str = "SLT boot_completed";

	if(!strncmp(bootanim_end_str, buf, strlen(bootanim_end_str)))
	{
		pr_info("[STRESS SLT] call slt_check_status\n");
		slt_check_status();
	}

	if(!strncmp(memtester_str, buf, strlen(memtester_str)))
	{
		if(!strncmp(memtester_pass_str, buf, strlen(memtester_pass_str)))
		{
			slt_set_mem_pass(1);
		}
		else
		{
			DAL_SetScreenColor(DAL_COLOR_RED);
			slt_set_mem_pass(0);
		}
	}

	if(!strncmp(stressapp_pass_str, buf, strlen(stressapp_pass_str)))
	{
		slt_set_stressapp_pass(1);
	}

	if(!strncmp(stressapp_fail_str, buf, strlen(stressapp_fail_str)))
	{
		DAL_SetScreenColor(DAL_COLOR_RED);
		slt_set_stressapp_pass(0);
	}

	pr_emerg("[STRESS SLT] %s\n", buf);
	SLT_DISP_PRINT("[GEM STRESS SLT] %s", buf);

	if (!MTK_EVB)
		mdelay(3000);
	return count;
}

DRIVER_ATTR(reboot_test, 0644, reboot_test_show, reboot_test_store);
DRIVER_ATTR(report, 0644, NULL, report_store);

int __init slt_reboot_init(void)
{
	int ret;
	ret = driver_register(&slt_reboot_drv.driver);
	if (ret) {
		pr_err("fail to create slt test driver\n");
	} else{
		pr_err("success to create slt test driver\n");
	}

	ret = driver_create_file(&slt_reboot_drv.driver, &driver_attr_reboot_test);
	if (ret) {
		pr_err("fail to create reboot_test sysfs files\n");
	}else{
		pr_err("success to create reboot_test sysfs files\n");
	}

	ret = driver_create_file(&slt_reboot_drv.driver, &driver_attr_report);
	if (ret) {
		pr_err("fail to create report sysfs files\n");
	}else{
		pr_err("success to create report sysfs files\n");
	}

	return 0;
}
arch_initcall(slt_reboot_init);


#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define WDT_STATUS_ABNORMAL 2

static int reboot_test_status_show(struct seq_file *m, void *v) {
	int wdt_status = 0;
	int in_reboot = slt_get_reboot_test_enabled();

	/* check wdt status for memtest */
	if (!in_reboot)
		wdt_status = slt_get_last_wdt_status();

	if ((wdt_status != 0) && (wdt_status != 2))
		seq_printf(m, "%d", WDT_STATUS_ABNORMAL);
	else
		seq_printf(m, "%d", in_reboot);

	return 0;
}

static int reboot_test_status_open(struct inode *inode, struct  file *file) {
	return single_open(file, reboot_test_status_show, NULL);
}

static const struct file_operations reboot_test_status_fops = {
	.owner = THIS_MODULE,
	.open = reboot_test_status_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init reboot_test_status_init(void) {
	proc_create("reboot_test_status", 0, NULL, &reboot_test_status_fops);
	return 0;
}

static void __exit reboot_test_status_exit(void) {
	remove_proc_entry("reboot_test_status", NULL);
}

module_init(reboot_test_status_init);
module_exit(reboot_test_status_exit);
