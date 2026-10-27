/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __PROSLIC_SYS_HDR2__
#define __PROSLIC_SYS_HDR2__ 1
#include <linux/types.h>
#include <linux/time.h> /* for struct timespec */

#include <linux/spi/spi.h>

#include "proslic_sys_cfg.h"
#include "proslic_sys.h"


/* Do a quick sanity check on  channel setting */
#define SILABS_MAX_CHAN 32 
#if (SILABS_MAX_CHANNELS > SILABS_MAX_CHAN)
#error "ProSLIC only supports up to 32 channels"
#endif

#define PROSLIC_BCAST       0xFF
#define PROSLIC_SPI_OK      0
#define PROSLIC_SPI_NOK     8 /* This is what the ProSLIC API calls SPI_FAIL */ 
#define PROSLIC_TIMER_ERROR 15

/* Debug macros */
#define PROSLIC_API_HDR "PROSLIC_API "

#define proslic_trace(fmt, arg...) if(proslic_debug_setting2 & 0x1) printk( KERN_NOTICE PROSLIC_API_HDR "TRC: " fmt "\n", ##arg)
#define proslic_debug(fmt, arg...) if(proslic_debug_setting2 & 0x2) printk( KERN_DEBUG PROSLIC_API_HDR "DBG: " fmt "\n", ##arg)
#define  proslic_error(fmt, arg...) if(proslic_debug_setting2 & 0x4) printk( KERN_ERR PROSLIC_API_HDR "ERR: " fmt "\n", ##arg)

/* Data types compatible with the ProSLIC API system services layer */
typedef u_int8_t            BOOLEAN;
typedef int8_t              int8;
typedef u_int8_t            uInt8;
typedef uInt8               uChar;
typedef int16_t             int16;
typedef u_int16_t           uInt16;
typedef int32_t             int32;
typedef u_int32_t           uInt32;
typedef u_int32_t           ramData;


#ifndef CTRL_H2
/* Function prototypes compatible with the ProSLIC API system services layer */
typedef int (*ctrl_Reset_fptr2) (void *hCtrl, int in_reset);

typedef int (*ctrl_WriteRegister_fptr2) (void *hCtrl, uInt8 channel, uInt8 regAddr, uInt8 data);
typedef int (*ctrl_WriteRAM_fptr2) (void *hCtrl, uInt8 channel, uInt16 ramAddr, ramData data);

typedef uInt8 (*ctrl_ReadRegister_fptr2) (void *hCtrl, uInt8 channel, uInt8 regAddr);
typedef ramData (*ctrl_ReadRAM_fptr2) (void *hCtrl, uInt8 channel, uInt16 ramAddr);

typedef int (*ctrl_Semaphore_fptr2) (void *hCtrl, int in_critical_section);
#endif

typedef void *(*get_hctrl) (uInt8 channel); /* Get a pointer to the control interface */

typedef int (*system_delay_fptr) (void *hTimer, int timeInMs);
typedef int (*system_timeElapsed_fptr) (void *hTimer, void *startTime, int *timeInMs);
typedef int (*system_getTime_fptr) (void *hTimer, void *time);
typedef void *(get_Timer)(void); /* return a pointer to "the" hTimer */

/* Function table entries to reduce namespace pollution */
typedef struct
{
  ctrl_Reset_fptr2             reset;

  ctrl_WriteRegister_fptr2     write_reg;
  ctrl_WriteRAM_fptr2          write_ram;

  ctrl_ReadRegister_fptr2      read_reg;
  ctrl_ReadRAM_fptr2           read_ram;

  ctrl_Semaphore_fptr2         sem;
} proslic_spi_fptrs_t2;

typedef struct
{
  system_delay_fptr          slic_delay;
  system_timeElapsed_fptr    elapsed_time;
  system_getTime_fptr        get_time;
} proslic_timer_fptrs_t2;

/* Timer container */
typedef struct 
{
  struct timespec64 timerObj;
} proslic_timeStamp2;

extern int proslic_channel_count2;
extern int proslic_debug_setting2;
int proslic_spi_setup2(void);
void proslic_spi_shutdown2(void);

/* Exported functions/values */
extern proslic_timer_fptrs_t proslic_timer_if;
extern proslic_spi_fptrs_t2 proslic_spi_if2; 
int proslic_reset2(void *hCtrl, int in_reset);
int proslic_write_register2(void *hCtrl, uInt8 channel, uInt8 regAddr, uInt8 data);
int proslic_write_ram2(void *hCtrl, uInt8 channel, uInt16 ramAddr, ramData data);
uInt8 proslic_read_register2(void *hCtrl, uInt8 channel, uInt8 regAddr);
ramData proslic_read_ram2(void *hCtrl, uInt8 channel, uInt16 ramAddr);

int proslic_get_channel_count2(void);
proslic_dev_t proslic_get_device_type2(uint8_t channel);
void *proslic_get_hCtrl2(uint8_t channel);
int proslic_spi_probe2(struct spi_device *spi, struct spi_driver *spi_drv);
int proslic_spi_remove2(struct spi_device *spi);


#endif /* End of __PROSLIC_SYS_HDR__ */

