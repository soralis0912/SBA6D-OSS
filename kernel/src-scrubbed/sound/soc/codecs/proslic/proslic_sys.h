/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __PROSLIC_SYS_HDR__
#define __PROSLIC_SYS_HDR__ 1
#include <linux/types.h>
#include <linux/time.h> /* for struct timespec */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>

#include <linux/spi/spi.h>

#include "proslic_sys_cfg.h"
#include "./inc/si_voice.h"
#include "./inc/proslic.h"



/* Do a quick sanity check on  channel setting */
#define SILABS_MAX_CHAN 32 
#if (SILABS_MAX_CHANNELS > SILABS_MAX_CHAN)
#error "ProSLIC only supports up to 32 channels"
#endif

#define PROSLIC_BCAST       0xFF
#define PROSLIC_SPI_OK      0
#define PROSLIC_SPI_NOK     8 /* This is what the ProSLIC API calls SPI_FAIL */ 
#define PROSLIC_TIMER_ERROR 15

#define PROSLIC_NUM_PORTS   1
#define PROSLIC_CHARDEV_BASE_NAME "proslic_interface"

/* Debug macros */
#define PROSLIC_API_HDR "PROSLIC_API "

#define proslic_trace(fmt, arg...) if(proslic_debug_setting & 0x1) printk( KERN_NOTICE PROSLIC_API_HDR "TRC: " fmt "\n", ##arg)
#define proslic_debug(fmt, arg...) if(proslic_debug_setting & 0x2) printk( KERN_DEBUG PROSLIC_API_HDR "DBG: " fmt "\n", ##arg)
#define  proslic_error(fmt, arg...) if(proslic_debug_setting & 0x4) printk( KERN_ERR PROSLIC_API_HDR "ERR: " fmt "\n", ##arg)

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


#ifndef CTRL_H
/* Function prototypes compatible with the ProSLIC API system services layer */
typedef int (*ctrl_Reset_fptr) (void *hCtrl, int in_reset);

typedef int (*ctrl_WriteRegister_fptr) (void *hCtrl, uInt8 channel, uInt8 regAddr, uInt8 data);
typedef int (*ctrl_WriteRAM_fptr) (void *hCtrl, uInt8 channel, uInt16 ramAddr, ramData data);

typedef uInt8 (*ctrl_ReadRegister_fptr) (void *hCtrl, uInt8 channel, uInt8 regAddr);
typedef ramData (*ctrl_ReadRAM_fptr) (void *hCtrl, uInt8 channel, uInt16 ramAddr);

typedef int (*ctrl_Semaphore_fptr) (void *hCtrl, int in_critical_section);
#endif

typedef void *(*get_hctrl) (uInt8 channel); /* Get a pointer to the control interface */

typedef int (*system_delay_fptr) (void *hTimer, int timeInMs);
typedef int (*system_timeElapsed_fptr) (void *hTimer, void *startTime, int *timeInMs);
typedef int (*system_getTime_fptr) (void *hTimer, void *time);
typedef void *(get_Timer)(void); /* return a pointer to "the" hTimer */

/* Function table entries to reduce namespace pollution */
typedef struct
{
  ctrl_Reset_fptr             reset;

  ctrl_WriteRegister_fptr     write_reg;
  ctrl_WriteRAM_fptr          write_ram;

  ctrl_ReadRegister_fptr      read_reg;
  ctrl_ReadRAM_fptr           read_ram;

  ctrl_Semaphore_fptr         sem;
} proslic_spi_fptrs_t;

typedef struct
{
  system_delay_fptr          slic_delay;
  system_timeElapsed_fptr    elapsed_time;
  system_getTime_fptr        get_time;
} proslic_timer_fptrs_t;

/* Timer container */
typedef struct 
{
  struct timespec64 timerObj;
} proslic_timeStamp;

extern int proslic_channel_count;
extern int proslic_debug_setting;
int proslic_spi_setup(void);
void proslic_spi_shutdown(void);

typedef enum
{
  PROSLIC_IS_UNKNOWN,
  PROSLIC_IS_PROSLIC,
  PROSLIC_IS_DAA
} proslic_dev_t;

/* Exported functions/values */
extern proslic_timer_fptrs_t proslic_timer_if;
extern proslic_spi_fptrs_t proslic_spi_if; 
int proslic_reset(void *hCtrl, int in_reset);
int proslic_write_register(void *hCtrl, uInt8 channel, uInt8 regAddr, uInt8 data);
int proslic_write_ram(void *hCtrl, uInt8 channel, uInt16 ramAddr, ramData data);
uInt8 proslic_read_register(void *hCtrl, uInt8 channel, uInt8 regAddr);
ramData proslic_read_ram(void *hCtrl, uInt8 channel, uInt16 ramAddr);

int proslic_get_channel_count(void);
proslic_dev_t proslic_get_device_type(uint8_t channel);
void *proslic_get_hCtrl(uint8_t channel);
int proslic_spi_probe(struct spi_device *spi, struct spi_driver *spi_drv);
int proslic_spi_remove(struct spi_device *spi);

typedef struct 
{
  uint8_t             lock_count;
  struct cdev         cdev;
  struct dev_t        *dev_t;
} proslic_file_dev_t;

typedef struct
{
  int breakMin;
  int breakMax;
  int makeMin;
  int makeMax;
  int interDigitMin;
  int flashMin;
  int flashMax;
  int hookMin;
} slic_hook_time;

typedef enum
{
  QUECTEL_SLIC_NO_EVENT,
  QUECTEL_SLIC_ONHOOK,  /**< Hook state is onhook      */
  QUECTEL_SLIC_OFFHOOK, /**< Hook state is offhook     */
  QUECTEL_SLIC_DTMF,    /**< Event status is DTMF      */
  QUECTEL_SLIC_FLASHKEY,/**< Event status is FLASHKEY  */
  QUECTEL_SLIC_MAX_NUM
}proslic_event_i;

typedef enum QL_SLIC_TONE_PRESET_TYPE_ENUM{
  QL_SLIC_NONE_TONE = 0,
  QL_SLIC_DTMF_1,
  QL_SLIC_DTMF_2,
  QL_SLIC_DTMF_3,
  QL_SLIC_DTMF_4,
  QL_SLIC_DTMF_5,
  QL_SLIC_DTMF_6,
  QL_SLIC_DTMF_7,
  QL_SLIC_DTMF_8,
  QL_SLIC_DTMF_9,
  QL_SLIC_DTMF_0,
  QL_SLIC_DTMF_STAR,
  QL_SLIC_DTMF_POUND,
  QL_SLIC_DTMF_A,
  QL_SLIC_DTMF_B,
  QL_SLIC_DTMF_C,
  QL_SLIC_DTMF_D,
  QL_SLIC_DIAL_TONE,
  QL_SLIC_DIAL2_TONE,
  QL_SLIC_BUSY_TONE,
  QL_SLIC_WAITING_TONE,
  QL_SLIC_RINGBACK_TONE,
  QL_SLIC_WHISTLING_TONE,
  QL_SLIC_TONE_MAX
}QL_SLIC_TONE_PRESET_TYPE_E;

typedef enum QL_SLIC_CALL_CID_MODE_ENUM{
  QL_CALL_CID_DTMF,
  QL_CALL_CID_FSK_SDMF,
  QL_CALL_CID_FSK_MDMF,
  QL_CALL_CID_FSK_VMWI_ON,
  QL_CALL_CID_FSK_VMWI_OFF,
  QL_CALL_CID_MODE_MAX
}QL_SLIC_CALL_CID_MODE_E;

#define DSCI_MAX_NUMBER_CHARS		21
#define DSCI_MAX_TIME_CHARS			9
#define DSCI_MAX_NAME_CHARS			11
#define FSK_DEPTH_TRIG    3

typedef struct 
{
  unsigned char slic_type;
  unsigned int country_code;

  proslic_event_i slic_event;
  unsigned char slic_dtmf;

  unsigned char tone_flag;
  unsigned char tone_preset;

  unsigned char call_num[DSCI_MAX_NUMBER_CHARS];
  unsigned char call_ring;

  QL_SLIC_CALL_CID_MODE_E call_cid_mode;
  unsigned char fsk_time[DSCI_MAX_TIME_CHARS];
  unsigned char fsk_name[DSCI_MAX_NAME_CHARS];

  slic_hook_time hook_time;

  bool is_reverse;
  int gain_AToD;
  int gain_DToA;
  unsigned char error;
  
  uInt8 send_buf[32];
  uInt8 recv_buf[32];
  uInt8 spi_len;

  bool is_slic_reset;
}quectel_slic_info;

extern int quectel_slic_id;
#define QUECTEL_SLIC_LE9643   1
#define QUECTEL_SLIC_SI321XX  2

void quectel_get_time(struct timespec *timers);

unsigned char quectel_si321xx_init(unsigned int country_code);
int quectel_si321xx_get_event(unsigned long ioctl_params);
int quectel_si321xx_set_tone(quectel_slic_info *slic_info);
int quectel_si321xx_set_dtmf_cid(quectel_slic_info *slic_info);
int quectel_si321xx_set_fsk_cid(quectel_slic_info *slic_info);
int quectel_si321xx_set_ring(quectel_slic_info *slic_info);
int quectel_si321xx_close_dev(void);
int quectel_si321xx_set_hook_time(quectel_slic_info *slic_info);
int quectel_si321xx_get_hook_time(unsigned long ioctl_params);
int quectel_si321xx_print_register(void);
int quectel_si321xx_set_talk_reverse(bool is_reverse);
int quectel_si321xx_set_gain(int16 gain_AToD, int16 gain_DToA);

int quectel_spi_write_read(uInt8 *cmd,uInt8 cmd_len,uInt8 *read_buf,uInt8 read_len);
unsigned char quectel_le9643_init(unsigned int country_code);
int quectel_le9643_get_event(unsigned long ioctl_params);
int quectel_le9643_set_tone(quectel_slic_info *slic_info);
int quectel_le9643_set_dtmf_cid(quectel_slic_info *slic_info);
int quectel_le9643_set_fsk_cid(quectel_slic_info *slic_info);
int quectel_le9643_set_ring(quectel_slic_info *slic_info);
int quectel_le9643_close_dev(void);
int quectel_le9643_set_hook_time(quectel_slic_info *slic_info);
int quectel_le9643_get_hook_time(unsigned long ioctl_params);
int quectel_le9643_print_register(void);
int quectel_le9643_set_talk_reverse(bool is_reverse);
int quectel_le9643_set_gain(int16 gain_AToD, int16 gain_DToA);

unsigned char quectel_slic_init(quectel_slic_info *slic_info);
int quectel_slic_get_event(unsigned long ioctl_params);
int quectel_slic_set_tone(quectel_slic_info *slic_info);
int quectel_slic_set_cid(quectel_slic_info *slic_info);
int quectel_slic_set_ring(quectel_slic_info *slic_info);
int quectel_slic_close_dev(void);
int quectel_slic_set_hook_time(quectel_slic_info *slic_info);
int quectel_slic_get_hook_time(unsigned long ioctl_params);
int quectel_slic_print_register(void);
int quectel_slic_set_talk_reverse(quectel_slic_info *slic_info);
int quectel_slic_set_gain(quectel_slic_info *slic_info);

#define SLIC_REG_PCMMODE    11
#define SLIC_REG_PCMTXLO    12
#define SLIC_REG_PCMTXHI    13
#define SLIC_REG_PCMRXLO    14
#define SLIC_REG_PCMRXHI    15

#define PROSLIC_MAGIC_IOCTL_NUM 123 /* MUST be a 8 bit number */
#define QUECTEL_IOCTL_SLIC_INIT					_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 0, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_GET_EVENT			_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 1, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_SET_RING				_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 2, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_SET_CALLNUM			_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 3, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_SET_CALLRING			_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 4, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_CLOSE_DEV			_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 5, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_SET_HOOK_TIME		_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 6, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_GET_HOOK_TIME		_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 7, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_PRINT_REGISTER		_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 8, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_SET_TALK_REVERSE		_IOWR(PROSLIC_MAGIC_IOCTL_NUM, 9, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_SET_GAIN		      _IOWR(PROSLIC_MAGIC_IOCTL_NUM, 10, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_SPI_DATA		      _IOWR(PROSLIC_MAGIC_IOCTL_NUM, 11, quectel_slic_info *)
#define QUECTEL_IOCTL_SLIC_RESET	          _IOWR(PROSLIC_MAGIC_IOCTL_NUM, 12, quectel_slic_info *)
#define PROSLIC_IOCTL_MAX_NUM 30

#define QUECTEL_SLIC_NO_ERROR              0
#define QUECTEL_SLIC_ERROR_PCMMODE_EFAULT  (EFAULT+SLIC_REG_PCMMODE)
#define QUECTEL_SLIC_ERROR_PCMTXLO_EFAULT  (EFAULT+SLIC_REG_PCMTXLO)
#define QUECTEL_SLIC_ERROR_PCMTXHI_EFAULT  (EFAULT+SLIC_REG_PCMTXHI)
#define QUECTEL_SLIC_ERROR_PCMRXLO_EFAULT  (EFAULT+SLIC_REG_PCMRXLO)
#define QUECTEL_SLIC_ERROR_PCMRXHI_EFAULT  (EFAULT+SLIC_REG_PCMRXHI)
#define QUECTEL_SLIC_ERROR_ENHANCE_EFAULT  (EFAULT+PROSLIC_REG_ENHANCE)
#define QUECTEL_SLIC_ERROR_HOOK_EVENT      200



#endif /* End of __PROSLIC_SYS_HDR__ */

