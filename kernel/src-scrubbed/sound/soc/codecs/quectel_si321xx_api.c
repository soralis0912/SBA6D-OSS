/**
 * @file quectel_slic_api.c
 * @brief Quectel SLIC function interface declarations.
 *
 * @note Provide API functions required by SLIC corresponding functions, include get_event, set_ring.
 */
/*============================================================================
  Copyright (c) 2021 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
 =============================================================================*/
/*============================================================================

							EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  WHEN             WHO         WHAT, WHERE, WHY
  ------------     -------     ----------------------------------------
  2021/03/24	   ricaro      Create file and add SLIC basic API
  ============================================================================*/

#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include "proslic/proslic_sys.h"
#include "proslic/si321xx.h"
#include "proslic/inc/proslic.h"
#include "proslic/config_inc/proslic_api_config.h"
#include "proslic/config_inc/quectel_tone_presets.h"

static hookChangeType hookdetection;
static struct timespec timers;
static int looking_for_pulse = 0;
static int hook_status_flag = QUECTEL_SLIC_NO_EVENT;

/* Pulse Dial/Hookflash Config */
static hookChange_Cfg hook_change_cfg = {	33,     //breakMin
											100,    //breakMax
											17,     //makeMin
											75,     //makeMax
											250,    //interDigitMin
											300,    //flashMin
											630,    //flashMax
											680     //onHookMin
											}; /* Min Idle Time */

typedef struct 
{
	char dtmfChar;
	char toneIndex;
} dtmfMap_t;

static dtmfMap_t dtmfMapping[] =
{
	{'0', QL_SLIC_DTMF_0},
	{'1', QL_SLIC_DTMF_1},
	{'2', QL_SLIC_DTMF_2},
	{'3', QL_SLIC_DTMF_3},
	{'4', QL_SLIC_DTMF_4},
	{'5', QL_SLIC_DTMF_5},
	{'6', QL_SLIC_DTMF_6},
	{'7', QL_SLIC_DTMF_7},
	{'8', QL_SLIC_DTMF_8},
	{'9', QL_SLIC_DTMF_9},
	{'A', QL_SLIC_DTMF_A},
	{'B', QL_SLIC_DTMF_B},
	{'C', QL_SLIC_DTMF_C},
	{'D', QL_SLIC_DTMF_D},
	{'*', QL_SLIC_DTMF_STAR},
	{'#', QL_SLIC_DTMF_POUND},
	{0,0}
};

struct SIProfileMapWithMCC_t{
	unsigned int m_MCC;
	int m_CID;
	ProSLIC_Tone_Cfg *m_Tone;
};

struct SIProfileMapWithMCC_t SIProfileMapWithMCC[] = {
	{460, DEFAULT_FSK, ProSLIC_CN_Chinese_standard_tone},
	{461, DEFAULT_FSK, ProSLIC_CN_Chinese_standard_tone},
	{262, ETSI_FSK, ProSLIC_DE_Germany_standard_tone},
	{214, ETSI_FSK, ProSLIC_ES_Spain_standard_tone},
	{234, ETSI_FSK, ProSLIC_UK_United_Kingdom_standard_tone},
	{235, ETSI_FSK, ProSLIC_UK_United_Kingdom_standard_tone},
	{724, ETSI_FSK, ProSLIC_BR_Brazil_standard_tone},
	{208, ETSI_FSK, ProSLIC_FR_France_standard_tone},
	{427, DEFAULT_FSK, ProSLIC_QA_Qatar_standard_tone},
};

int default_CID = 0;

unsigned char quectel_si321xx_init(unsigned int country_code)
{
	unsigned char error = 0;
	int i = 0;

	error = si321xx_ProSLIC_HWInit();
	if(error) {
		LOGPRINT("HW Init error\n");
		return error;
	}
	// for(i=0;i<NUMBER_OF_CHAN;i++)
	// {
	// 	ProSLIC_PrintDebugReg(arrayOfProslicChans[i]);
	// }
	error = ProSLIC_EnableInterrupts(arrayOfProslicChans[0]);
	if(error) {
		LOGPRINT("Enable Interrupts error\n");
		return error;
	}
	quectel_get_time(&timers);
	error = ProSLIC_InitializeHookChangeDetect(&hookdetection,&timers);
	if(error) {
		LOGPRINT("InitializeHookChangeDetect error\n");
		return error;
	}
	LOGPRINT("End of initialization\n");
	// for(i=0;i<NUMBER_OF_CHAN;i++)
	// {
	// 	ProSLIC_PrintDebugReg(arrayOfProslicChans[i]);
	// }

	for(i=0;i<(sizeof(SIProfileMapWithMCC)/sizeof(struct SIProfileMapWithMCC_t));i++)
	{
		if(country_code == SIProfileMapWithMCC[i].m_MCC)
		{
			Quectel_Tone_Presets = SIProfileMapWithMCC[i].m_Tone;
			default_CID = SIProfileMapWithMCC[i].m_CID;
			break;
		}
	}
	if((Quectel_Tone_Presets == NULL) || (country_code == 0))
	{
		Quectel_Tone_Presets = ProSLIC_CN_Chinese_standard_tone;
		default_CID = DEFAULT_FSK;
	}
	error = ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0],LF_FWD_ACTIVE);
	if(error != RC_NONE) {
		LOGPRINT("Error running VpSetLineState on channel 0: %d\n", error);
	}
	return error;
}

int quectel_si321xx_get_event(unsigned long ioctl_params)
{
	int ret = -1;
	unsigned char error = 0;
	uInt8 event = 255;
	uInt8 reg_irq1 = 255;
	uInt8 dtmf = 255;
	uInt8 old_event = 255;
	proslicIntType irqs;
	quectel_slic_info slic_send_info = {0};

	memset(&slic_send_info,0,sizeof(quectel_slic_info));

	slic_send_info.slic_event = QUECTEL_SLIC_NO_EVENT;
	slic_send_info.error      = QUECTEL_SLIC_NO_ERROR;
	ProSLIC_GetInterrupts(arrayOfProslicChans[0],&irqs);

	if(irqs.number > 0)
	{
		LOGPRINT("Get event irqs.irqsm=0x%X     irqs.number =0x%X\n",*irqs.irqs,irqs.number);
		if((*irqs.irqs) == IRQ_LOOP_STATUS || (*irqs.irqs) == IRQ_VBAT) {
			ProSLIC_ReadHookStatus(arrayOfProslicChans[0], &event);
			if((event != PROSLIC_ONHOOK) && (event != PROSLIC_OFFHOOK)) {
				LOGPRINT("Read Hook status error\n");
				error = QUECTEL_SLIC_ERROR_HOOK_EVENT;
			}else {
				LOGPRINT("Get event: %d\n",event);

			}
			if(*irqs.irqs == IRQ_VBAT) {
				reg_irq1 = ProSLIC_ReadReg(arrayOfProslicChans[0],PROSLIC_REG_IRQ1);

				LOGPRINT("Get IRQ1: 0x%X\n",reg_irq1);
			}
		}
		if(*irqs.irqs == IRQ_DTMF) {
			ProSLIC_DTMFReadDigit(arrayOfProslicChans[0],&dtmf);
			slic_send_info.slic_event = QUECTEL_SLIC_DTMF;
			slic_send_info.slic_dtmf = (unsigned char)dtmf;
			LOGPRINT("Get DTMF: 0x%X\n",dtmf);
		}
	}
	if(event != old_event) {
		LOGPRINT("event: 0x%X  old_event: 0x%X\n",event,old_event);
		old_event = event;
		looking_for_pulse = 1;

		ret = ProSLIC_HookChangeDetect(arrayOfProslicChans[0],&hook_change_cfg,&hookdetection);
		if(ret == SI_HC_HOOKFLASH) {
			LOGPRINT("detect Hook flash\n");
			looking_for_pulse = 0;
			slic_send_info.slic_event = QUECTEL_SLIC_FLASHKEY;
			hook_status_flag = QUECTEL_SLIC_FLASHKEY;
		} else if (ret == SI_HC_OFFHOOK_TIMEOUT) {
			LOGPRINT("Off hook qualified\n");
			looking_for_pulse = 0;
			slic_send_info.slic_event = QUECTEL_SLIC_OFFHOOK;
			hook_status_flag = QUECTEL_SLIC_OFFHOOK;
		}
	}
	if(looking_for_pulse == 1) {
		ret = ProSLIC_HookChangeDetect(arrayOfProslicChans[0],&hook_change_cfg,&hookdetection);
		if(ret != SI_HC_NEED_MORE_POLLS) {
			looking_for_pulse = 0;
			if( SI_HC_DIGIT_DONE(ret) ) {
				LOGPRINT("Pulses/Digit detected: %d\n",ret);
			}
			else {
				switch(ret) {
					case SI_HC_ONHOOK_TIMEOUT:
						LOGPRINT("Onhook qualified\n");
						slic_send_info.slic_event = QUECTEL_SLIC_ONHOOK;
						hook_status_flag = QUECTEL_SLIC_ONHOOK;
						break;
					case SI_HC_OFFHOOK_TIMEOUT:
						LOGPRINT("Offhook qualified \n");
						slic_send_info.slic_event = QUECTEL_SLIC_OFFHOOK;
						hook_status_flag = QUECTEL_SLIC_OFFHOOK;
						break;
					case SI_HC_HOOKFLASH:
						LOGPRINT("Hook flash qualified\n");
						slic_send_info.slic_event = QUECTEL_SLIC_FLASHKEY;
						hook_status_flag = QUECTEL_SLIC_FLASHKEY;
						break;
					default:
						break;
				}
			}
		}
	}
	slic_send_info.error = error;
	if(copy_to_user((void *) __user ioctl_params, &slic_send_info, sizeof(quectel_slic_info)) != 0)
	{
		return -EFAULT;
	}
	return 0;
}

int quectel_si321xx_set_tone(quectel_slic_info *slic_info)
{
	uInt8 tone_preset = 255;
	uInt8 tone_flag = 255;

	tone_preset = slic_info->tone_preset;
	tone_flag   = slic_info->tone_flag;

	if( tone_preset >= QL_SLIC_TONE_MAX ) {
		LOGPRINT("Tone_Preset >= QL_SLIC_TONE_MAX\n");
		return -1;
	}
	if( ( tone_flag != 0 ) && ( tone_flag != 1 )) {
		LOGPRINT("tone_flag != 0 && tone_flag != 1\n");
	}
	if(tone_flag == 1) {
		LOGPRINT("tone_preset = %d \n",tone_preset);
		switch(tone_preset) {
			case QL_SLIC_NONE_TONE:
			case QL_SLIC_DTMF_1:
			case QL_SLIC_DTMF_2:
			case QL_SLIC_DTMF_3:
			case QL_SLIC_DTMF_4:
			case QL_SLIC_DTMF_5:
			case QL_SLIC_DTMF_6:
			case QL_SLIC_DTMF_7:
			case QL_SLIC_DTMF_8:
			case QL_SLIC_DTMF_9:
			case QL_SLIC_DTMF_0:
			case QL_SLIC_DTMF_STAR:
			case QL_SLIC_DTMF_POUND:
			case QL_SLIC_DTMF_A:
			case QL_SLIC_DTMF_B:
			case QL_SLIC_DTMF_C:
			case QL_SLIC_DTMF_D:
			case QL_SLIC_DIAL_TONE:
				/* The timer is always off */
				ProSLIC_ToneGenSetup(arrayOfProslicChans[0], tone_preset);
				ProSLIC_ToneGenStart(arrayOfProslicChans[0], FALSE);
				break;
			case QL_SLIC_DIAL2_TONE:
			case QL_SLIC_BUSY_TONE:
			case QL_SLIC_WAITING_TONE:
			case QL_SLIC_RINGBACK_TONE:
			case QL_SLIC_WHISTLING_TONE:
				/* The timer is always on */
				ProSLIC_ToneGenSetup(arrayOfProslicChans[0], tone_preset);
				ProSLIC_ToneGenStart(arrayOfProslicChans[0], TRUE);
				break;
			default :
				LOGPRINT("Unknow cmd\n");
				break;
		}
	} else if(tone_flag == 0) {
		ProSLIC_ToneGenStop(arrayOfProslicChans[0]);
	}
	
	return 0;
}

int quectel_si321xx_set_dtmf_cid(quectel_slic_info *slic_info)
{
	int i = 0, ret = 0;
	char *call_num = NULL;

	ret = ProSLIC_DisableInterrupts(arrayOfProslicChans[0]);
	if(ret) {
		LOGPRINT("Disable Interrupts error\n");
		return ret;
	}
	call_num = slic_info->call_num;
	LOGPRINT("set call num: %s\n",call_num);
	ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0],LF_FWD_OHT);
	mdelay(100);
	while(*call_num) {
		for(i = 0; dtmfMapping[i].dtmfChar; i++)
		{
			if( dtmfMapping[i].dtmfChar == *call_num )
			{
				ProSLIC_ToneGenSetup(arrayOfProslicChans[0], dtmfMapping[i].toneIndex);
				break;
			}
		}
		ProSLIC_ToneGenStart(arrayOfProslicChans[0], FALSE);
		mdelay(60);
		ProSLIC_ToneGenStop(arrayOfProslicChans[0]);
		mdelay(60);
		call_num++;
	}
	ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0],LF_FWD_ACTIVE);
	ret = ProSLIC_EnableInterrupts(arrayOfProslicChans[0]);
	if(ret) {
		LOGPRINT("Enable Interrupts error\n");
		return ret;
	}
	return 0;
}

static uInt8 checkSum(uInt8 *str)
{
	uInt8 i = 0;
	uInt8 nLen;
	unsigned short sum=0;

	nLen = str[1]+2;
	for(i = 0; i < nLen; i++) {
		sum += (unsigned short)str[i];
	}
	return ((uInt8)(256-(sum&0xff)));
}

static int waitForCIDDone(void)
{
	uInt8 avail = 0;
	do {
		ProSLIC_CheckCIDBuffer(arrayOfProslicChans[0], &avail);
	}while(avail == 0);

	return 0;
}

static int DoCallerIDwithCRCMessage(uInt8 *message)
{
	int ret = 1;
	int i;
	int length = 0;
	uInt8 ucbuff[300]={0};

	length = 0;
	for(i = 0;i < 38;i++) {//Chinese is 300 bit 0&1
		ucbuff[length] = 0xAA;
		length++;
	}

	for(i = 0;i < 24;i++) {//Chinese is 180 '1' + 1~10 flag
		ucbuff[length] = 0xFF;
		length++;
	}

	for(i = 0;i < (message[1]+3);i++) {//with CRC (message[1]+3) <<== length in message[1]
		if(length > 290) {
			LOGPRINT("Send Message length too long , and length =%3d\n",(message[1]+3));
			break;
		}
		ucbuff[length] = 0x7f;
		length++;
		ucbuff[length] = message[i];
		length++;
	}

	for(i = 0;i < 7;i++) {//Chinese is 180 '1' + 1~10 flag
		ucbuff[length]=0xFF;
		length++;
	}
	/*  */
	ProSLIC_FSKSetup(arrayOfProslicChans[0], default_CID);
	ProSLIC_ModifyCIDStartBits(arrayOfProslicChans[0], 1);
	ProSLIC_EnableCID(arrayOfProslicChans[0]);
	ProSLIC_SendCID(arrayOfProslicChans[0], ucbuff, 8);
	for (i = 8;i < length;i += FSK_DEPTH_TRIG) {
		if(i >= FSK_DEPTH_TRIG) {//send 8 bytes to re-fill fifo
			ret = waitForCIDDone();
		}
		if(ProSLIC_SendCID(arrayOfProslicChans[0], &(ucbuff[i]), FSK_DEPTH_TRIG) != RC_NONE)
		{
			ProSLIC_DisableCID(arrayOfProslicChans[0]);
			return -1;
		}
	}
	ProSLIC_DisableCID(arrayOfProslicChans[0]);
	ProSLIC_ModifyCIDStartBits(arrayOfProslicChans[0], 0);
	return 0;
}

int quectel_si321xx_set_fsk_cid(quectel_slic_info *slic_info)
{
	uInt8 cidMessage[256] = {0};
	int message_p = 0;
	uInt8 data;
	unsigned char default_time[] = "01010000";

	if(strlen(slic_info->fsk_time) != 8) 
	{
		memset(slic_info->fsk_time,0,sizeof(slic_info->fsk_time));
		strncpy(slic_info->fsk_time, default_time, strlen(default_time)); /* 01/01 00:00 */
	}
	
	switch(slic_info->call_cid_mode) {
		case QL_CALL_CID_FSK_SDMF:
			LOGPRINT("FSK_SDMF CID mode\n");
			message_p = 0;
			cidMessage[message_p] = 0x04;                  /* Message Type SDMF */
			message_p++;
			//cidMessage[1]:                               /* Message Length */
			message_p++;

			/* SDMF: Time is a must */
			LOGPRINT("fsk_time = %s\n", slic_info->fsk_time);
			strncpy(&cidMessage[message_p], slic_info->fsk_time, strlen(slic_info->fsk_time));
			message_p += strlen(slic_info->fsk_time);

			if(slic_info->call_num == NULL) {
				LOGPRINT("Number is not available\n");
				cidMessage[message_p] = 'O';               /* Number is not available */
				message_p++;
			} else if(!strcmp(slic_info->call_num, "anonymous")) {
				LOGPRINT("Private Number\n");
				cidMessage[message_p] = 'P';               /* Private Number */
				message_p++;
			} else {
				LOGPRINT("call_num = %s\n", slic_info->call_num);
				strncpy(&cidMessage[message_p], slic_info->call_num, strlen(slic_info->call_num));   /* Number */
				message_p += strlen(slic_info->call_num);
			}
			cidMessage[1] = message_p - 2;                 /* 2 = cidMessage[0] + cidMessage[1] */
			break;
		case QL_CALL_CID_FSK_MDMF:
			LOGPRINT("FSK_MDMF CID mode\n");
			message_p = 0;
			cidMessage[message_p] = 0x80;                  /* Message Type MDMF */
			message_p++;
			//cidMessage[1]:                               /* Message Length */
			message_p++;

			if(strlen(slic_info->fsk_time) != 0) {
				LOGPRINT("fsk_time = %s, length = %ld\n", slic_info->fsk_time, strlen(slic_info->fsk_time));
				cidMessage[message_p] = 0x01;              /* Parameter Type - Time */
				message_p++;
				cidMessage[message_p] = strlen(slic_info->fsk_time);      /* Parameter Length */
				message_p++;
				strncpy(&cidMessage[message_p], slic_info->fsk_time, strlen(slic_info->fsk_time));
				message_p += strlen(slic_info->fsk_time);
			}

			if(slic_info->call_num == NULL) {
				LOGPRINT("Number is not available\n");
				cidMessage[message_p] = 0x04;              /* Parameter Type - call num - number not present */
				message_p++;
				cidMessage[message_p] = 1;                 /* Parameter Length */
				message_p++;
				cidMessage[message_p] = 'O';               /* Number is not available */
				message_p++;
			} else if(!strcmp(slic_info->call_num, "anonymous")) {
				LOGPRINT("Private Number\n");
				cidMessage[message_p] = 0x04;              /* Parameter Type - call num - number not present */
				message_p++;
				cidMessage[message_p] = 1;                 /* Parameter Length */
				message_p++;
				cidMessage[message_p] = 'P';               /* Private Number */
				message_p++;
			} else {
				LOGPRINT("call_num = %s, length = %ld\n", slic_info->call_num, strlen(slic_info->call_num));
				cidMessage[message_p] = 0x02;              /* Parameter Type - call num */
				message_p++;
				cidMessage[message_p] = strlen(slic_info->call_num);  /* Parameter Length */
				message_p++;
				strncpy(&cidMessage[message_p], slic_info->call_num, strlen(slic_info->call_num));   /* Number */
				message_p += strlen(slic_info->call_num);
			}

			if(strlen(slic_info->fsk_name) == 0) {
				LOGPRINT("Name is not available\n");
				cidMessage[message_p] = 0x08;              /* Parameter Type - call name - name not present */
				message_p++;
				cidMessage[message_p] = 1;                 /* Parameter Length */
				message_p++;
				cidMessage[message_p] = 'O';               /* Number is not available */
				message_p++;
			} else if(!strcmp(slic_info->fsk_name, "anonymous")) {
				LOGPRINT("Private name\n");
				cidMessage[message_p] = 0x08;              /* Parameter Type - call name - name not present */
				message_p++;
				cidMessage[message_p] = 1;                 /* Parameter Length */
				message_p++;
				cidMessage[message_p] = 'P';               /* Private Number */
				message_p++;
			} else {
				LOGPRINT("name = %s, length = %ld\n", slic_info->fsk_name,strlen(slic_info->fsk_name));
				cidMessage[message_p] = 0x07;              /* Parameter Type - call name */
				message_p++;
				cidMessage[message_p] = strlen(slic_info->fsk_name);      /* Parameter Length */
				message_p++;
				strncpy(&cidMessage[message_p], slic_info->fsk_name, strlen(slic_info->fsk_name));   /* name */
				message_p += strlen(slic_info->fsk_name);
			}
			cidMessage[1] = message_p - 2;                 /* 2 = cidMessage[0] + cidMessage[1] */
			break;
		case QL_CALL_CID_FSK_VMWI_ON:
		case QL_CALL_CID_FSK_VMWI_OFF:
			message_p = 0;
			cidMessage[0] = 0x82;                          /* Message Type - Message Waiting notification ==> TNA102 */
			message_p++;
			message_p++;
			if(strlen(slic_info->fsk_time) != 0) {
				LOGPRINT("fsk_time = %s\n", slic_info->fsk_time);
				cidMessage[message_p] = 0x01;              /* Parameter Type - Time */
				message_p++;
				cidMessage[message_p] = strlen(slic_info->fsk_time);      /* Parameter Length */
				message_p++;
				strncpy(&cidMessage[message_p], slic_info->fsk_time, strlen(slic_info->fsk_time));
				message_p += strlen(slic_info->fsk_time);
			}
			cidMessage[message_p] = 0x0B;                  /* Parameter Type - Visual Indicator */
			message_p++;
			cidMessage[message_p] = 1;                     /* Parameter Length */
			message_p++;
			if(slic_info->call_cid_mode == QL_CALL_CID_FSK_VMWI_ON) {
				cidMessage[message_p] = 0xFF;
			} else {
				cidMessage[message_p] = 0x00;
			}
			message_p += 1;
			cidMessage[1] = message_p - 2;                 /* 2 = cidMessage[0] + cidMessage[1] */
			break;
		default :
			LOGPRINT("unknow type\n");
			return -1;
	}

	data = checkSum(cidMessage);
	cidMessage[cidMessage[1]+2] = data;
	ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0], LF_FWD_OHT);
	if(DoCallerIDwithCRCMessage(cidMessage) < 0)
	{
		LOGPRINT("send FSK CID failed!!!\n");
		return -1;
	}
	ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0], LF_FWD_ACTIVE);
	return 0;
}

int quectel_si321xx_set_ring(quectel_slic_info *slic_info)
{
	uInt8 call_ring = 255;

	call_ring = slic_info->call_ring;
	if(call_ring !=0 && call_ring != 1) {
		LOGPRINT("call_ring != 0 && call_ring != 1\n");
	}
	if(call_ring == 1) {
		ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0],LF_RINGING);
	} else if(call_ring == 0) {
		ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0],LF_FWD_ACTIVE);
	}
	return 0;
}

int quectel_si321xx_close_dev(void)
{
	int ret = 0;

	LOGPRINT("Enter quectel_si321xx_close_dev\n");
	ret = ProSLIC_PCMStop(arrayOfProslicChans[0]);
	if(ret) {
		LOGPRINT("ProSLIC_PCMStop error\n");
	}
	return ret;
}

int quectel_si321xx_set_hook_time(quectel_slic_info *slic_info)
{
	LOGPRINT("breakMin = %d breakMax = %d makeMin = %d makeMax = %d interDigitMin = %d \
		flashMin = %d flashMax = %d hookMin = %d \n", slic_info->hook_time.breakMin, \
			slic_info->hook_time.breakMax, slic_info->hook_time.makeMin, \
			slic_info->hook_time.makeMax, slic_info->hook_time.interDigitMin, \
			slic_info->hook_time.flashMin, slic_info->hook_time.flashMax, \
			slic_info->hook_time.hookMin);

	if((slic_info->hook_time.breakMin < 0 || slic_info->hook_time.breakMin > 2000)
		|| (slic_info->hook_time.breakMax < 0 || slic_info->hook_time.breakMax > 2000)
		|| (slic_info->hook_time.makeMin < 0 || slic_info->hook_time.makeMin > 2000)
		|| (slic_info->hook_time.makeMax < 0 || slic_info->hook_time.makeMax > 2000)
		|| (slic_info->hook_time.interDigitMin < 0 || slic_info->hook_time.interDigitMin > 2000)
		|| (slic_info->hook_time.flashMin < 0 || slic_info->hook_time.flashMin > 2000)
		|| (slic_info->hook_time.flashMax < 0 || slic_info->hook_time.flashMax > 2000)
		|| (slic_info->hook_time.hookMin < 0 || slic_info->hook_time.hookMin > 2000)) {
		
		LOGPRINT("The set value does not meet the range (0~2000 ms)\n");
		return -1;
	}

	hook_change_cfg.minOnHook = slic_info->hook_time.breakMin;
	hook_change_cfg.maxOnHook = slic_info->hook_time.breakMax;
	hook_change_cfg.minOffHook = slic_info->hook_time.makeMin;
	hook_change_cfg.maxOffHook = slic_info->hook_time.makeMax;
	hook_change_cfg.minInterDigit = slic_info->hook_time.interDigitMin;
	hook_change_cfg.minHookFlash = slic_info->hook_time.flashMin;
	hook_change_cfg.maxHookFlash = slic_info->hook_time.flashMax; 
	hook_change_cfg.minHook = slic_info->hook_time.hookMin; 
	
	return 0;
}

int quectel_si321xx_get_hook_time(unsigned long ioctl_params)
{
	quectel_slic_info slic_send_info = {0};
	
	LOGPRINT("Enter quectel_si321xx_get_hook_time\n");
	slic_send_info.hook_time.breakMin = hook_change_cfg.minOnHook;
	slic_send_info.hook_time.breakMax = hook_change_cfg.maxOnHook;
	slic_send_info.hook_time.makeMin = hook_change_cfg.minOffHook;
	slic_send_info.hook_time.makeMax = hook_change_cfg.maxOffHook;
	slic_send_info.hook_time.interDigitMin = hook_change_cfg.minInterDigit;
	slic_send_info.hook_time.flashMin = hook_change_cfg.minHookFlash;
	slic_send_info.hook_time.flashMax = hook_change_cfg.maxHookFlash;
	slic_send_info.hook_time.hookMin = hook_change_cfg.minHook;

	LOGPRINT("breakMin = %d breakMax = %d makeMin = %d makeMax = %d interDigitMin = %d \
		flashMin = %d flashMax = %d hookMin = %d \n", slic_send_info.hook_time.breakMin, \
			slic_send_info.hook_time.breakMax, slic_send_info.hook_time.makeMin, \
			slic_send_info.hook_time.makeMax, slic_send_info.hook_time.interDigitMin, \
			slic_send_info.hook_time.flashMin, slic_send_info.hook_time.flashMax, \
			slic_send_info.hook_time.hookMin);
	if(copy_to_user((void *) __user ioctl_params, &slic_send_info, sizeof(quectel_slic_info)) != 0)
	{
		return -EFAULT;
	}
	return 0;
}

int quectel_si321xx_print_register(void)
{
	int i=0;
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_PrintDebugReg(arrayOfProslicChans[i]);
	}
	return 0;
}

int quectel_si321xx_set_talk_reverse(bool is_reverse)
{
	int error;
	if(hook_status_flag == QUECTEL_SLIC_OFFHOOK) 
	{
		if(is_reverse == true)
		{
			LOGPRINT("QUECTEL_SLIC_OFFHOOK LF_REV_ACTIVE\n");
			error = ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0],LF_REV_ACTIVE);
			if(error != RC_NONE) {
				LOGPRINT("Error running VpSetLineState on channel 0: %d\n", error);
			}
		}
		else
		{
			LOGPRINT("QUECTEL_SLIC_OFFHOOK LF_FWD_ACTIVE\n");
			error = ProSLIC_SetLinefeedStatus(arrayOfProslicChans[0],LF_FWD_ACTIVE);
			if(error != RC_NONE) {
				LOGPRINT("Error running VpSetLineState on channel 0: %d\n", error);
			}
		}
	}
	else
	{
		LOGPRINT("Slic status not HOOK OFF, set talk reverse failed!\n");
		error = -1;
	}
	return error;
}

int quectel_si321xx_set_gain(int16 gain_AToD, int16 gain_DToA)
{
	int ret;
	if(gain_AToD<-6 || gain_AToD>6) return -1;
	if(gain_DToA<-12 || gain_DToA>0) return -1;
	ret = ProSLIC_AudioGainSetup(arrayOfProslicChans[0], gain_DToA, gain_AToD, 0);
	if(ret != RC_NONE) {
		LOGPRINT("Error running ProSLIC_AudioGain on channel 0: %d\n", ret);
	}
	return ret;
}
