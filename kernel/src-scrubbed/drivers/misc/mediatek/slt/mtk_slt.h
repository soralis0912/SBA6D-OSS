#ifndef __MTK_SLT_H__
#define __MTK_SLT_H__


#include <linux/delay.h>
//#include <mt-plat/mtk_thermal_monitor.h>

extern void wdt_arch_reset(char mode);

extern u8 slt_get_reboot_test_enabled(void);
extern void slt_set_reboot_test_enabled(u8 val);
extern u64 slt_get_reboot_count(void);
extern void slt_set_reboot_count(u64 val);
extern u32 slt_get_last_wdt_status(void);
extern void slt_set_mem_pass(u8 val);
extern u8 slt_get_mem_pass(void);
extern void slt_set_stressapp_pass(u8 val);
extern u8 slt_get_stressapp_pass(void);
void slt_check_status(void);
extern void slt_ram_mem_pass_sync(void);
extern void slt_ram_stressapp_pass_sync(void);
#endif

