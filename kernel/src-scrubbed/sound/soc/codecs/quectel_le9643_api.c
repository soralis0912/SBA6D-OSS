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
  2021/04/22	   ricaro      Create file and add SLIC basic API
  ============================================================================*/
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include "le9643_api/inc/vp_api.h"
#include "le9643_api/inc/vp_api_common.h"
#include "le9643_api/inc/profiles.h"
#include "proslic/config_inc/proslic_api_config.h"
#include "../mediatek/mt6990/mt6990-afe-common.h"

#define NUM_LINES       1
/* ZTAP specific values for device ID depending on the MPI Mode */
#define MPI_3WIRE       0x0000 /* 3 wire SPI w/ 2.5us interbyte chipselect off-time */
#define MPI_4WIRE       0x0100 /* 4 wire SPI w/ 2.5us interbyte chipselect off-time */
#define SPI8_3WIRE      0x0200 /* 3 wire SPI w/ one clock chipselect off-time */
#define SPI8_4WIRE      0x0400 /* 4 wire SPI w/ one clock chipselect off-time */
#define SPI8_3WIRE_CFM  0x0300 /* 3 wire SPI w/o chipselect off-time per VpMpiCmd() transation */
#define SPI8_4WIRE_CFM  0x0500 /* 4 wire SPI w/o chipselect off-time per VpMpiCmd() transation */
#define ZSI_4WIRE       0x0E00 /* 4 wire ZSI transation*/
#define SM              0x0010
#define DIN             0x0000

VpStatusType status;
VpDevCtxType devCtx;
VpDeviceIdType deviceId = (VpDeviceIdType)(DIN | SPI8_4WIRE);
VpLineCtxType lineCtx[NUM_LINES];
Vp886DeviceObjectType devObj;
Vp886LineObjectType lineObj[NUM_LINES];
uint8 cidPos = 0;
uint8 cidLength = 0;
char cidMessage[50] = {0};

struct LEProfileMapWithMCC_t{
	unsigned int MCC;
	const VpProfileDataType *m_Ringing;
	const VpProfileDataType *m_RingCadence;
	const VpProfileDataType *m_FSK_CID;
	const VpProfileDataType *m_FSK_CIDII;
	const VpProfileDataType (*m_Tone)[20];
	const VpProfileDataType (*m_Cadence)[40];
};

struct LEProfileMapWithMCC_t LEProfileMapWithMCC[] = {
   {460, RING_miSLIC_BB_DEF, CR_CN, CID_TYPE1_CN_FSK, CID_TYPE2_CN_FSK, CN_Chinese_standard_tone, CN_Chinese_standard_cadence},
   {461, RING_miSLIC_BB_DEF, CR_CN, CID_TYPE1_CN_FSK, CID_TYPE2_CN_FSK, CN_Chinese_standard_tone, CN_Chinese_standard_cadence},
   {262, RING_miSLIC_BB_DE, CR_DE, CID_TYPE1_DE_FSK, CID_TYPE2_DE_FSK, DE_Germany_standard_tone, DE_Germany_standard_cadence},
   {214, RING_miSLIC_BB_DE, CR_DE, CID_TYPE1_DE_FSK, CID_TYPE2_DE_FSK, ES_Spain_standard_tone, ES_Spain_standard_cadence},
   {234, RING_miSLIC_BB_DE, CR_DE, CID_TYPE1_DE_FSK, CID_TYPE2_DE_FSK, UK_United_Kingdom_standard_tone, UK_United_Kingdom_standard_cadence},
   {235, RING_miSLIC_BB_DE, CR_DE, CID_TYPE1_DE_FSK, CID_TYPE2_DE_FSK, UK_United_Kingdom_standard_tone, UK_United_Kingdom_standard_cadence},
   {724, RING_miSLIC_BB_DE, CR_DE, CID_TYPE1_DE_FSK, CID_TYPE2_DE_FSK, BR_Brazil_standard_tone, BR_Brazil_standard_cadence},
   {208, RING_miSLIC_BB_DE, CR_DE, CID_TYPE1_DE_FSK, CID_TYPE2_DE_FSK, FR_France_standard_tone, FR_France_standard_cadence},
   {427, RING_miSLIC_BB_DEF, CR_CN, CID_TYPE1_CN_FSK, CID_TYPE2_CN_FSK, QA_Qatar_standard_tone, QA_Qatar_standard_cadence},
};
struct LEProfileMapWithMCC_t *pLEProfileMapWithMCC = NULL;

slic_hook_time le9643_hook_time_info = {20,     //breakMin
										80,    //breakMax
										17,     //makeMin
										75,     //makeMax
										250,    //interDigitMin
										80,    //flashMin
										1030,    //flashMax
										1080     //onHookMin
										};

static int hook_status_flag = VP_LINE_EVID_HOOK_ON;
int quectel_le9643_get_event(unsigned long ioctl_params);  

char *MapStatus(VpStatusType status)
{
	int idx = 0;
	static char buff[50];
	static const char *strTable[VP_STATUS_NUM_TYPES] = {
		"VP_STATUS_SUCCESS",
		"VP_STATUS_FAILURE",
		"VP_STATUS_FUNC_NOT_SUPPORTED",
		"VP_STATUS_INVALID_ARG",
		"VP_STATUS_MAILBOX_BUSY",
		"VP_STATUS_ERR_VTD_CODE",
		"VP_STATUS_OPTION_NOT_SUPPORTED",
		"VP_STATUS_ERR_VERIFY",
		"VP_STATUS_DEVICE_BUSY",
		"VP_STATUS_MAILBOX_EMPTY",
		"VP_STATUS_ERR_MAILBOX_DATA",
		"VP_STATUS_ERR_HBI",
		"VP_STATUS_ERR_IMAGE",
		"VP_STATUS_IN_CRTCL_SECTN",
		"VP_STATUS_DEV_NOT_INITIALIZED",
		"VP_STATUS_ERR_PROFILE",
		"VP_STATUS_INVALID_VOICE_STREAM",
		"VP_STATUS_CUSTOM_TERM_NOT_CFG",
		"VP_STATUS_DEDICATED_PINS",
		"VP_STATUS_INVALID_LINE",
		"VP_STATUS_LINE_NOT_CONFIG",
		"VP_STATUS_ERR_SPI"
	};
	if (status < VP_STATUS_NUM_TYPES) {
		idx = sprintf(&buff[idx], "(%s)", (char *)strTable[status]);
	}
	idx = sprintf(&buff[idx], " = %d", (uint16)status);
	return buff;
}
void UtilPrintEvent(VpEventType *pEvent)
{
	VpEventCategoryType eventCategory = pEvent->eventCategory;
	uint16 eventId = pEvent->eventId;
	char *eventCategorySt[] = {
		"VP_EVCAT_FAULT",
		"VP_EVCAT_SIGNALING",
		"VP_EVCAT_RESPONSE",
		"VP_EVCAT_TEST",
		"VP_EVCAT_PROCESS",
		"VP_EVCAT_FXO",
		"VP_EVCAT_PACKET",
	};
	if (eventCategory < VP_NUM_EVCATS) {
		LOGPRINT("eventCategory = %s\n", eventCategorySt[eventCategory]);
	} else {
		LOGPRINT("eventCategory = INVALID (0x%x)\n", eventCategory);
	}
	if (eventCategory == VP_EVCAT_FAULT) {
		switch (eventId) {
			case VP_DEV_EVID_BAT_FLT:
				LOGPRINT("VP_DEV_EVID_BAT_FLT\n"); break;
			case VP_DEV_EVID_CLK_FLT:
				LOGPRINT("VP_DEV_EVID_CLK_FLT\n"); break;
			case VP_LINE_EVID_THERM_FLT:
				LOGPRINT("VP_LINE_EVID_THERM_FLT\n"); break;
			case VP_LINE_EVID_DC_FLT:
				LOGPRINT("VP_LINE_EVID_DC_FLT\n"); break;
			case VP_LINE_EVID_AC_FLT:
				LOGPRINT("VP_LINE_EVID_AC_FLT\n"); break;
			case VP_DEV_EVID_EVQ_OFL_FLT:
				LOGPRINT("VP_DEV_EVID_EVQ_OFL_FLT\n"); break;
			case VP_DEV_EVID_WDT_FLT:
				LOGPRINT("VP_DEV_EVID_WDT_FLT\n"); break;
			default:
				LOGPRINT("INVALID (0x%x)\n", eventId); break;
		}
	} else if (eventCategory == VP_EVCAT_SIGNALING) {
		switch (eventId) {
			case VP_LINE_EVID_HOOK_OFF:
				LOGPRINT("VP_LINE_EVID_HOOK_OFF\n"); break;
			case VP_LINE_EVID_HOOK_ON:
				LOGPRINT("VP_LINE_EVID_HOOK_ON\n"); break;
			case VP_LINE_EVID_GKEY_DET:
				LOGPRINT("VP_LINE_EVID_GKEY_DET\n"); break;
			case VP_LINE_EVID_GKEY_REL:
				LOGPRINT("VP_LINE_EVID_GKEY_REL\n"); break;
			case VP_LINE_EVID_FLASH:
				LOGPRINT("VP_LINE_EVID_FLASH\n"); break;
			case VP_LINE_EVID_STARTPULSE:
				LOGPRINT("VP_LINE_EVID_STARTPULSE\n"); break;
			case VP_LINE_EVID_DTMF_DIG:
				LOGPRINT("VP_LINE_EVID_DTMF_DIG\n"); break;
			case VP_LINE_EVID_PULSE_DIG:
				LOGPRINT("VP_LINE_EVID_PULSE_DIG\n"); break;
			case VP_LINE_EVID_MTONE:
				LOGPRINT("VP_LINE_EVID_MTONE\n"); break;
			case VP_DEV_EVID_TS_ROLLOVER:
				LOGPRINT("VP_DEV_EVID_TS_ROLLOVER\n"); break;
			default:
				LOGPRINT("INVALID (0x%x)\n", eventId); break;
		}
	} else if (eventCategory == VP_EVCAT_RESPONSE) {
		switch (eventId) {
			case VP_DEV_EVID_BOOT_CMP:
				LOGPRINT("VP_DEV_EVID_BOOT_CMP\n"); break;
			case VP_LINE_EVID_LLCMD_TX_CMP:
				LOGPRINT("VP_LINE_EVID_LLCMD_TX_CMP\n"); break;
			case VP_LINE_EVID_LLCMD_RX_CMP:
				LOGPRINT("VP_LINE_EVID_LLCMD_RX_CMP\n"); break;
			case VP_DEV_EVID_DNSTR_MBOX:
				LOGPRINT("VP_DEV_EVID_DNSTR_MBOX\n"); break;
			case VP_LINE_EVID_RD_OPTION:
				LOGPRINT("VP_LINE_EVID_RD_OPTION\n"); break;
			case VP_LINE_EVID_RD_LOOP:
				LOGPRINT("VP_LINE_EVID_RD_LOOP\n"); break;
			case VP_EVID_CAL_CMP:
				LOGPRINT("VP_EVID_CAL_CMP\n"); break;
			case VP_EVID_CAL_BUSY:
				LOGPRINT("VP_EVID_CAL_BUSY\n"); break;
			case VP_LINE_EVID_GAIN_CMP:
				LOGPRINT("VP_LINE_EVID_GAIN_CMP\n"); break;
			case VP_DEV_EVID_DEV_INIT_CMP:
				LOGPRINT("VP_DEV_EVID_DEV_INIT_CMP\n"); break;
			case VP_LINE_EVID_LINE_INIT_CMP:
				LOGPRINT("VP_LINE_EVID_LINE_INIT_CMP\n"); break;
			case VP_DEV_EVID_IO_ACCESS_CMP:
				LOGPRINT("VP_DEV_EVID_IO_ACCESS_CMP\n"); break;
			default:
				LOGPRINT("INVALID (0x%x)\n", eventId); break;
		}
	} else if (eventCategory == VP_EVCAT_TEST) {
		switch (eventId) {
			case VP_LINE_EVID_TEST_CMP:
				LOGPRINT("VP_LINE_EVID_TEST_CMP\n"); break;
			case VP_LINE_EVID_DTONE_DET:
				LOGPRINT("VP_LINE_EVID_DTONE_DET\n"); break;
			case VP_LINE_EVID_DTONE_LOSS:
				LOGPRINT("VP_LINE_EVID_DTONE_LOSS\n"); break;
			case VP_DEV_EVID_STEST_CMP:
				LOGPRINT("VP_DEV_EVID_STEST_CMP\n"); break;
			case VP_DEV_EVID_CHKSUM:
				LOGPRINT("VP_DEV_EVID_CHKSUM\n"); break;
			default:
				LOGPRINT("INVALID (0x%x)\n", eventId); break;
		}
	} else if(eventCategory == VP_EVCAT_PROCESS) {
		switch (eventId) {
			case VP_LINE_EVID_MTR_CMP:
				LOGPRINT("VP_LINE_EVID_MTR_CMP\n"); break;
			case VP_LINE_EVID_MTR_ABORT:
				LOGPRINT("VP_LINE_EVID_MTR_ABORT\n"); break;
			case VP_LINE_EVID_CID_DATA:
				LOGPRINT("VP_LINE_EVID_CID_DATA\n"); break;
			case VP_LINE_EVID_RING_CAD:
				LOGPRINT("VP_LINE_EVID_RING_CAD\n"); break;
			case VP_LINE_EVID_GEN_TIMER:
				LOGPRINT("VP_LINE_EVID_GEN_TIMER\n"); break;
			default:
				LOGPRINT("INVALID (0x%x)\n", eventId); break;
		}
	} else if(eventCategory == VP_EVCAT_FXO) {
		switch (eventId) {
			case VP_LINE_EVID_RING_ON:
				LOGPRINT("VP_LINE_EVID_RING_ON\n"); break;
			case VP_LINE_EVID_RING_OFF:
				LOGPRINT("VP_LINE_EVID_RING_OFF\n"); break;
			case VP_LINE_EVID_LIU:
				LOGPRINT("VP_LINE_EVID_LIU\n"); break;
			case VP_LINE_EVID_LNIU:
				LOGPRINT("VP_LINE_EVID_LNIU\n"); break;
			case VP_LINE_EVID_FEED_DIS:
				LOGPRINT("VP_LINE_EVID_FEED_DIS\n"); break;
			case VP_LINE_EVID_FEED_EN:
				LOGPRINT("VP_LINE_EVID_FEED_EN\n"); break;
			case VP_LINE_EVID_DISCONNECT:
				LOGPRINT("VP_LINE_EVID_DISCONNECT\n"); break;
			case VP_LINE_EVID_RECONNECT:
				LOGPRINT("VP_LINE_EVID_RECONNECT\n"); break;
			case VP_LINE_EVID_POLREV:
				LOGPRINT("VP_LINE_EVID_POLREV\n"); break;
			case VP_LINE_EVID_POH:
				LOGPRINT("VP_LINE_EVID_POH\n"); break;
			case VP_LINE_EVID_PNOH:
				LOGPRINT("VP_LINE_EVID_PNOH\n"); break;
			default:
				LOGPRINT("INVALID (0x%x)\n", eventId); break;
		}
	} else {
		LOGPRINT(" 0x%x\n", eventId);
	}
}

void InitSetMasks(void)
{
	VpOptionEventMaskType eventMask = {0};
	VpOptionCodecType codecType = {0};
	VpOptionFsyncRateType rateType = {0};
	VpOptionPulseModeType pulseMode = VP_OPTION_PULSE_DECODE_ON;
	VpOptionPulseType pulseType = {0};
	VpOptionTimeslotType solt0 = {0};

	if(QUECTEL_ETDM_RATE == 16000)
	{
		LOGPRINT("LE9643 set option for Wideband, 16K\n");
		codecType = VP_OPTION_LINEAR_WIDEBAND;
		rateType = VP_FSYNC_RATE_16KHZ;
	}
	else if(QUECTEL_ETDM_RATE == 8000)
	{
		LOGPRINT("LE9643 set option for Narrowband, 8K\n");
		codecType =VP_OPTION_LINEAR;
		rateType = VP_FSYNC_RATE_8KHZ;
	}

	/* mask everything */
	VpMemSet(&eventMask, 0xFF, sizeof(VpOptionEventMaskType));

	/* unmask only the events the application is interested in */
	eventMask.faults = VP_EVCAT_FAULT_UNMASK_ALL;
	eventMask.signaling = VP_EVCAT_SIGNALING_UNMASK_ALL | VP_DEV_EVID_TS_ROLLOVER;
	eventMask.response = VP_EVCAT_RESPONSE_UNMASK_ALL;
	eventMask.test = VP_EVCAT_TEST_UNMASK_ALL;
	eventMask.process = VP_EVCAT_PROCESS_UNMASK_ALL;

	/* inform the API of the mask */
	status = VpSetOption(VP_NULL, &devCtx, VP_OPTION_ID_EVENT_MASK, &eventMask);
	status = VpSetOption(&lineCtx[0], NULL,VP_OPTION_ID_CODEC, &codecType);
	//status = VpSetOption(&lineCtx[0], NULL,VP_DEVICE_OPTION_ID_FSYNC_RATE, &rateType);
	status = VpSetOption(&lineCtx[0], VP_NULL, VP_OPTION_ID_PULSE_MODE, &pulseMode);

	LOGPRINT("breakMin = %d breakMax = %d makeMin = %d makeMax = %d interDigitMin = %d \
		flashMin = %d flashMax = %d HookMin = %d \n", le9643_hook_time_info.breakMin, \
			le9643_hook_time_info.breakMax, le9643_hook_time_info.makeMin, \
			le9643_hook_time_info.makeMax, le9643_hook_time_info.interDigitMin, \ 
			le9643_hook_time_info.flashMin, le9643_hook_time_info.flashMax, \
			le9643_hook_time_info.hookMin);

	pulseType.breakMin = le9643_hook_time_info.breakMin * 8; /* 33 ms */
	pulseType.breakMax = le9643_hook_time_info.breakMax * 8; /* 100ms */
	pulseType.makeMin = le9643_hook_time_info.makeMin * 8; /* 17ms */
	pulseType.makeMax = le9643_hook_time_info.makeMax * 8; /* 75ms */
	pulseType.interDigitMin = le9643_hook_time_info.interDigitMin * 8; /* 250ms */
	pulseType.flashMin = le9643_hook_time_info.flashMin * 8; /* modify from 150ms to 40 ms*/
	pulseType.flashMax = le9643_hook_time_info.flashMax * 8; /* 1000ms Requirement: Flash 40-1000ms */
	pulseType.onHookMin = le9643_hook_time_info.hookMin * 8; /* 1300ms */
	VpSetOption(VP_NULL, &devCtx, VP_DEVICE_OPTION_ID_PULSE, &pulseType);

	solt0.rx=0x00;
	solt0.tx=0x00;
	status = VpSetOption(&lineCtx[0], NULL,VP_OPTION_ID_TIMESLOT, &solt0);

	return;
}

static void ContinueCid(uint8 channelId)
{
	uint8 length = 0;
	int ret = 0;

	if(cidLength > 16) {
		length = 16;
		cidLength -= 16;
	} else {
		length = cidLength;
		cidLength = 0;
	}
	LOGPRINT("running VpContinueCid length %d %d\n", cidLength, length);
	ret = VpContinueCid(&lineCtx[0], length, &cidMessage[cidPos]);
	if(ret != VP_STATUS_SUCCESS) {
		LOGPRINT("Error running VpContinueCid on channel %d: %s\n", channelId, MapStatus(status));
	}
	cidPos += length;
}

unsigned char quectel_le9643_init(unsigned int country_code)
{
	uint8 lineNum = 0;
	uInt8 error = 255;
	int interval_time = 0;
	int i = 0;
	struct timespec timers1 = {0};
	struct timespec timers2 = {0};

	for(i=0;i<(sizeof(LEProfileMapWithMCC)/sizeof(struct LEProfileMapWithMCC_t));i++)
	{
		if(country_code == LEProfileMapWithMCC[i].MCC)
		{
			pLEProfileMapWithMCC = &(LEProfileMapWithMCC[i]);
			break;
		}
	}
	if((pLEProfileMapWithMCC == NULL) || (country_code == 0))
	{
		pLEProfileMapWithMCC = &(LEProfileMapWithMCC[0]);
	}
	LOGPRINT("Init with MCC: %d\n", pLEProfileMapWithMCC->MCC);

	error = VpMakeDeviceObject(VP_DEV_887_SERIES, deviceId, &devCtx, &devObj);
	if (error != VP_STATUS_SUCCESS) {
		LOGPRINT("Error making the device object: %s\n", MapStatus(error));
		return error;
	}
	for (; lineNum < NUM_LINES; lineNum++) {
		error = VpMakeLineObject(VP_TERM_FXS_LOW_PWR, lineNum, &lineCtx[lineNum], &lineObj[lineNum], &devCtx);
		if (error != VP_STATUS_SUCCESS) {
			LOGPRINT("Error calling VpMakeLineObject on lineNum %i: %s\n", lineNum, MapStatus(error));
			return error;
		}
		error = VpMapLineId(&lineCtx[lineNum], lineNum);
		if (error != VP_STATUS_SUCCESS) {
			LOGPRINT("Error calling VpMapLineId on lineNum %i: %s\n", lineNum, MapStatus(error));
			return error;
		}
		LOGPRINT(" --- Created lineNum %i---\n", lineNum);
	}
	//VpRegisterDump(&devCtx);
	LOGPRINT("Load profile:LE9643.\n");
	error = VpInitDevice(&devCtx, miSLIC_IB_DEV_PROFILE, AC_FXS_RF14_600_ABS,                     //Customers pls modify here base on hardware difference
						DC_FXS_miSLIC_BB_DEF, pLEProfileMapWithMCC->m_Ringing, VP_PTABLE_NULL, VP_PTABLE_NULL);
	if (error != VP_STATUS_SUCCESS) {
		LOGPRINT("Device not properly initialized: %s\n", MapStatus(error));
		return error;
	} else {
		LOGPRINT("Device Initialization started\n");
	}
	//VpRegisterDump(&devCtx);
	while(1) {
		quectel_get_time(&timers1);
		error = quectel_le9643_get_event(0);
		if(error == 1) {
			LOGPRINT("Device Initialization\n");
			break;
		}
		mdelay(10);
		quectel_get_time(&timers2);
		interval_time = (timers2.tv_sec * 1000 + timers2.tv_nsec / 1000000) - (timers1.tv_sec * 1000 + timers1.tv_nsec / 1000000);
		if(interval_time > 1000) {
			LOGPRINT("Time out, interval_time = %d\n",interval_time);
			break;
		}
	}
	if(error > 0) {
		LOGPRINT("END\n");
		error = VpSetLineState(&lineCtx[0], VP_LINE_TALK);
		if(error != VP_STATUS_SUCCESS) {
			LOGPRINT("Error running VpSetLineState on channel 0: %s\n", MapStatus(status));
		}
		return 0;
	}
	LOGPRINT("END(error)!\n");
	return error;
}

int quectel_le9643_get_event(unsigned long ioctl_params)
{
	uint8 lineNum = 0;
	uInt8 error = 255;
	VpEventType event;
	uint8 cmpCounter = 0;
	VpOptionDtmfModeType dtmfMode = {0};
	quectel_slic_info slic_send_info = {0};

	slic_send_info.slic_event = QUECTEL_SLIC_NO_EVENT;
	slic_send_info.error      = QUECTEL_SLIC_NO_ERROR;

	while(VpGetEvent(&devCtx, &event)) {
		UtilPrintEvent(&event);
		if(event.eventCategory == VP_EVCAT_RESPONSE) {
			if(event.eventId == VP_DEV_EVID_DEV_INIT_CMP) {
				LOGPRINT("event.eventData=%d\n",event.eventData);
				InitSetMasks();
				LOGPRINT("Device Initialization completed\n");
				for(; lineNum < NUM_LINES; lineNum++) {
					LOGPRINT("VpSetLineState VP_LINE_TALK\n");
					error = VpSetLineState(&lineCtx[lineNum], VP_LINE_TALK);
					if (error != VP_STATUS_SUCCESS) {
						LOGPRINT("Error setting lineNum %i to VP_LINE_TALK: %s\n", lineNum, MapStatus(status));
						return -1;
					}
					error = VpCalLine(&lineCtx[lineNum]);
					LOGPRINT("Starting Calibration on lineNum %i\n", lineNum);
					if (error != VP_STATUS_SUCCESS) {
						LOGPRINT("Error calling VpCalLine on lineNum %i:: %s\n", lineNum, MapStatus(status));
						return -1;
					}
				}
			} else if(event.eventId == VP_EVID_CAL_CMP) {
				if (++cmpCounter == NUM_LINES) {
					LOGPRINT("Calibration completed on both lines\n");
					error = VpInitRing(&lineCtx[0], pLEProfileMapWithMCC->m_RingCadence, CID_TYPE1_CN_DTMF);
					if (error != VP_STATUS_SUCCESS) {
						LOGPRINT("Error running VpInitRing on channel %d: %s\n", 0, MapStatus(status));
						return -1;
					}
					LOGPRINT("SLIC device initialized successfully\n");
					return 1;
				}
			}
		} else if(event.eventCategory == VP_EVCAT_SIGNALING) {
			if(event.eventId == VP_LINE_EVID_HOOK_OFF) {
				dtmfMode.dtmfControlMode = VP_OPTION_DTMF_DECODE_ON;
				dtmfMode.direction = VP_DIRECTION_US;
				hook_status_flag = VP_LINE_EVID_HOOK_OFF;
				error = VpSetOption(VP_NULL, &devCtx, VP_OPTION_ID_DTMF_MODE, &dtmfMode);
				LOGPRINT("VpSetLineState VP_LINE_TALK\n");
				error = VpSetLineState(event.pLineCtx, VP_LINE_TALK);
				if(error != VP_STATUS_SUCCESS) {
					LOGPRINT("Error running VpSetLineState on channel %d: %s\n", event.channelId, MapStatus(status));
				}
				slic_send_info.slic_event = QUECTEL_SLIC_OFFHOOK;
			} else if(event.eventId == VP_LINE_EVID_HOOK_ON) {
				dtmfMode.dtmfControlMode = VP_OPTION_DTMF_DECODE_OFF;
				dtmfMode.direction = VP_DIRECTION_US;
				hook_status_flag = VP_LINE_EVID_HOOK_ON;
				error = VpSetOption(VP_NULL, &devCtx, VP_OPTION_ID_DTMF_MODE, &dtmfMode);
				slic_send_info.slic_event = QUECTEL_SLIC_ONHOOK;
			} else if(event.eventId == VP_LINE_EVID_DTMF_DIG) {
				if((event.eventData & 0x0010) == VP_DIG_SENSE_BREAK) {
					slic_send_info.slic_event = QUECTEL_SLIC_DTMF;
					slic_send_info.slic_dtmf = (unsigned char)(event.eventData & 0x000F);
					LOGPRINT("slic_send_info.slic_event = %d  slic_send_info.slic_dtmf = %d \n",slic_send_info.slic_event,slic_send_info.slic_dtmf);
				}
			} else if(event.eventId == VP_LINE_EVID_FLASH) {
				LOGPRINT("Got flash key\n");
				slic_send_info.slic_event = QUECTEL_SLIC_FLASHKEY;
			}
			if(ioctl_params)
			{
				if(copy_to_user((void *) __user ioctl_params, &slic_send_info, sizeof(quectel_slic_info)) != 0)
				{
					return -EFAULT;
				}
			}
				return 0;
		} else if(event.eventCategory == VP_EVCAT_PROCESS) {
			if(event.eventId == VP_LINE_EVID_CID_DATA) {
				if (event.eventData == VP_CID_DATA_TX_DONE) {
					LOGPRINT("Caller ID transmission has ended on channel %d\n", event.channelId);
				} else if(event.eventData == VP_CID_DATA_NEED_MORE_DATA) {
					ContinueCid(event.channelId);
				}
				return 0;
			}
		}  else if(event.eventCategory == VP_EVCAT_FAULT) {
			if(event.eventId == VP_DEV_EVID_CLK_FLT) {
				VpFreeRun(&devCtx, VP_FREE_RUN_START);
				msleep(500);
				error = VpInitDevice(&devCtx, miSLIC_IB_DEV_PROFILE, AC_FXS_RF14_600_ABS,                     //Customers pls modify here base on hardware difference
						DC_FXS_miSLIC_BB_DEF, pLEProfileMapWithMCC->m_Ringing, VP_PTABLE_NULL, VP_PTABLE_NULL);
				if (error != VP_STATUS_SUCCESS) {
					LOGPRINT("Device not properly initialized: %s\n", MapStatus(error));
					return error;
				} else {
					LOGPRINT("Device Initialization started\n");
				}
				return 0;
			}
		}
	}
	return 0;
}

int quectel_le9643_set_tone(quectel_slic_info *slic_info)
{
	uInt8 tone_preset = 255;
	uInt8 tone_flag = 255;
	uInt8 error = 255;

	tone_flag   = slic_info->tone_flag;
	tone_preset = slic_info->tone_preset;

	if( tone_preset >= QL_SLIC_TONE_MAX ) {
		LOGPRINT("Tone_Preset >= QL_SLIC_TONE_MAX\n");
		return -1;
	}
	if(( tone_flag != 0 ) && ( tone_flag != 1 )) {
		LOGPRINT("tone_flag != 0 && tone_flag != 1\n");
	}

	error = VpSetLineState(&lineCtx[0], VP_LINE_TALK);
	if(error != VP_STATUS_SUCCESS) {
		LOGPRINT("Error running VpSetLineState on channel 0: %s\n", MapStatus(status));
	}
	if( tone_flag == 1) {
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
			case QL_SLIC_DIAL2_TONE:
			case QL_SLIC_BUSY_TONE:
			case QL_SLIC_WAITING_TONE:
			case QL_SLIC_RINGBACK_TONE:
			case QL_SLIC_WHISTLING_TONE:
			{
				error = VpSetLineTone(&lineCtx[0], pLEProfileMapWithMCC->m_Tone[tone_preset], pLEProfileMapWithMCC->m_Cadence[tone_preset],VP_NULL);
				if(error) {
					LOGPRINT("Failed to set dial tone\n");
					return error;
				}
				break;
			}
			default :
				LOGPRINT("Unknow cmd\n");
				break;
		}
	} else if(tone_flag == 0) {
		error = VpSetLineTone(&lineCtx[0], VP_NULL, VP_NULL, VP_NULL);
		if(error) {
			LOGPRINT("Failed to set dial tone\n");
			return error;
		}
	}
	return error;
}

int quectel_le9643_set_dtmf_cid(quectel_slic_info *slic_info)
{
	int ret = 255;
	uint8 length;
	cidPos = 0;

	if(strlen(slic_info->call_num) == 0) {
		LOGPRINT("call_num is NULL\n");
		return -1;
	}
	LOGPRINT("DTMF CID mode\n");
	LOGPRINT("set call num: %s\n",slic_info->call_num);
	memset(cidMessage, 0, sizeof(cidMessage));
	snprintf(cidMessage,sizeof(cidMessage),"D%s",slic_info->call_num);
	
	cidLength = strlen(cidMessage);
	if(cidLength >= 32) {
		length = 32;
		cidLength -= 32;
	} else {
		length = cidLength;
		cidLength = 0;
	}
	if(hook_status_flag == VP_LINE_EVID_HOOK_ON)
	{
		LOGPRINT("DTMF CID type I,on hook\n");
		ret = VpInitRing(&lineCtx[0], pLEProfileMapWithMCC->m_RingCadence, CID_TYPE1_CN_DTMF);
		if(ret != VP_STATUS_SUCCESS) {
			LOGPRINT("Error running VpInitRing on channel %d: %s\n", 0, MapStatus(ret));
		}
		LOGPRINT("Starting Ringing & Type-I CID on ch0, length %d (remaining %d)\n", length, cidLength);
		ret = VpInitCid(&lineCtx[0], length, &cidMessage[cidPos]);
		if(ret != VP_STATUS_SUCCESS) {
		LOGPRINT("Error running VpInitCid on channel 0: %s\n", MapStatus(ret));
		return ret;
	}
	}
	else //OFF-HOOK
	{
		LOGPRINT("DTMF CID type II,off hook\n");
		ret = VpSendCid(&lineCtx[0], length, CID_TYPE2_CN_DTMF, &cidMessage[cidPos]);
		if(ret != VP_STATUS_SUCCESS) {
			LOGPRINT("Failed to VpSendCid\n");
			return ret;
		}
	}

	cidPos += length;
	return ret;
}

int quectel_le9643_set_fsk_cid(quectel_slic_info *slic_info)
{
	int ret = 0;
	uint8 length = 0;
	int message_p = 0;
	unsigned char default_time[] = "01010000";
	cidPos = 0;
	cidLength = 0;

	LOGPRINT("FSK CID mode\n");
	if(strlen(slic_info->fsk_time) != 8) 
	{
		memset(slic_info->fsk_time,0,sizeof(slic_info->fsk_time));
		strncpy(slic_info->fsk_time, default_time, strlen(default_time)); /* 01/01 00:00 */
	}

	memset(cidMessage, 0, sizeof(cidMessage));
	switch(slic_info->call_cid_mode) {
		case QL_CALL_CID_FSK_SDMF:
			LOGPRINT("FSK_SDMF CID mode\n");
			message_p = 0;
			cidMessage[message_p] = 0x04;                  /* Message Type SDMF */
			message_p++;
			//cidMessage[1]:                               /* Message Length */
			message_p++;

			LOGPRINT("time = %s\n", slic_info->fsk_time);
			strncpy(&cidMessage[message_p], slic_info->fsk_time, strlen(slic_info->fsk_time));
			message_p += strlen(slic_info->fsk_time);
			
			if(strlen(slic_info->call_num) == 0) {
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

			cidMessage[message_p] = 0x11;                   /*Message call type*/
			message_p++;
			cidMessage[message_p] = 0x01;                   /*Message call type length*/
			message_p++;
			cidMessage[message_p] = 0x01;
			message_p++;
			
			if(strlen(slic_info->fsk_time) != 0) {
				LOGPRINT("time = %s\n", slic_info->fsk_time);
				cidMessage[message_p] = 0x01;              /* Parameter Type - Time */
				message_p++;
				cidMessage[message_p] = strlen(slic_info->fsk_time);      /* Parameter Length */
				message_p++;
				strncpy(&cidMessage[message_p], slic_info->fsk_time, strlen(slic_info->fsk_time));
				message_p += strlen(slic_info->fsk_time);
			}

			if(strlen(slic_info->call_num) == 0) {
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
				LOGPRINT("call_num = %s\n", slic_info->call_num);
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
				LOGPRINT("name = %s\n", slic_info->fsk_name);
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
				LOGPRINT("time = %s\n", slic_info->fsk_time);
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

	cidLength = message_p;
	if(cidLength >= 32) {
		length = 32;
		cidLength -= 32;
	} else {
		length = cidLength;
		cidLength = 0;
	}

	if(hook_status_flag == VP_LINE_EVID_HOOK_ON)
	{
		LOGPRINT("FSK CID type I,on hook\n");
		if ( (slic_info->call_cid_mode == QL_CALL_CID_FSK_VMWI_ON) || (slic_info->call_cid_mode == QL_CALL_CID_FSK_VMWI_OFF) ) { // VMWI not need ring, 
			LOGPRINT("VpSetLineState VP_LINE_OHT\n");
			ret = VpSetLineState(&lineCtx[0], VP_LINE_OHT);
			if (ret != VP_STATUS_SUCCESS) {
				LOGPRINT("Failed to set VP_LINE_OHT\n");
				return ret;
			}
			ret = VpSendCid(&lineCtx[0], length, pLEProfileMapWithMCC->m_FSK_CID, &cidMessage[cidPos]);
			if(ret != VP_STATUS_SUCCESS) {
				LOGPRINT("Failed to VpSendCid\n");
				return ret;
			}
		}
		else
		{
			ret = VpInitRing(&lineCtx[0], pLEProfileMapWithMCC->m_RingCadence, pLEProfileMapWithMCC->m_FSK_CID);
			if(ret != VP_STATUS_SUCCESS) {
				LOGPRINT("ERROR: unable to init ring %d\n",ret);
			}
			LOGPRINT("Starting Ringing & Type-I CID on ch0, length %d (remaining %d)\n", length, cidLength);
			ret = VpInitCid(&lineCtx[0], length, &cidMessage[cidPos]);
			if(ret != VP_STATUS_SUCCESS) {
				LOGPRINT("Error running VpInitCid on channel 0: %s\n", MapStatus(ret));
				return ret;
			}
		}
	}
	else //OFF-HOOK
	{
		LOGPRINT("FSK CID type II,off hook\n");
		ret = VpSendCid(&lineCtx[0], length, pLEProfileMapWithMCC->m_FSK_CIDII, &cidMessage[cidPos]);
		if(ret != VP_STATUS_SUCCESS) {
			LOGPRINT("Failed to VpSendCid\n");
			return ret;
		}
	}

	cidPos += length;
	return ret;
}


int quectel_le9643_set_ring(quectel_slic_info *slic_info)
{
	uInt8 error = 255;
	uInt8 call_ring = 255;

	call_ring = slic_info->call_ring;
	LOGPRINT("call_ring = %d\n",call_ring);
	if(call_ring !=0 && call_ring != 1) {
		LOGPRINT("call_ring != 0 && call_ring != 1\n");
	}
	if(call_ring == 1) {
		LOGPRINT("VpSetLineState VP_LINE_RINGING\n");
		error = VpSetLineState(&lineCtx[0], VP_LINE_RINGING);
		if (error != VP_STATUS_SUCCESS) {
			LOGPRINT("Failed to set VP_LINE_RINGING\n");
			return error;
		}
	} else if(call_ring == 0) {
		if(hook_status_flag == VP_LINE_EVID_HOOK_OFF) {
			LOGPRINT("VpSetLineState QUECTEL_SLIC_OFFHOOK VP_LINE_TALK\n");
			error = VpSetLineState(&lineCtx[0], VP_LINE_TALK);
			if(error != VP_STATUS_SUCCESS) {
				LOGPRINT("Error running VpSetLineState on channel 0: %s\n", MapStatus(status));
			}
		} else if(hook_status_flag == VP_LINE_EVID_HOOK_ON) {
			LOGPRINT("VpSetLineState QUECTEL_SLIC_ONHOOK VP_LINE_STANDBY\n");
			error = VpSetLineState(&lineCtx[0], VP_LINE_STANDBY);
			if(error != VP_STATUS_SUCCESS) {
				LOGPRINT("Error running VpSetLineState on channel 0: %s\n", MapStatus(status));
			}
		}

		if(error != VP_STATUS_SUCCESS) {
			LOGPRINT("Failed to set VP_LINE_STANDBY\n");
			return error;
		}
	}
	return error;
}

int quectel_le9643_close_dev(void)
{
	uInt8 error = 255;

	LOGPRINT("Enter quectel_le9643_close_dev\n");
	error = VpSetLineState(&lineCtx[0], VP_LINE_DISABLED);
	if(error != VP_STATUS_SUCCESS) {
		LOGPRINT("Error running VpSetLineState on channel 0: %s\n", MapStatus(status));
	}
	return error;
}

int quectel_le9643_set_hook_time(quectel_slic_info *slic_info)
{
	VpOptionPulseType pulseType = {0};

	LOGPRINT("breakMin = %d breakMax = %d makeMin = %d makeMax = %d interDigitMin = %d \
		flashMin = %d flashMax = %d HookMin = %d \n", slic_info->hook_time.breakMin, \
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

	pulseType.breakMin = slic_info->hook_time.breakMin * 8;
	pulseType.breakMax = slic_info->hook_time.breakMax * 8;
	pulseType.makeMin = slic_info->hook_time.makeMin * 8;
	pulseType.makeMax = slic_info->hook_time.makeMax * 8;
	pulseType.interDigitMin = slic_info->hook_time.interDigitMin * 8;
	pulseType.flashMin = slic_info->hook_time.flashMin * 8;
	pulseType.flashMax = slic_info->hook_time.flashMax * 8;
	pulseType.onHookMin = slic_info->hook_time.hookMin * 8;
	VpSetOption(VP_NULL, &devCtx, VP_DEVICE_OPTION_ID_PULSE, &pulseType);

	le9643_hook_time_info.breakMin = slic_info->hook_time.breakMin;
	le9643_hook_time_info.breakMax = slic_info->hook_time.breakMax;
	le9643_hook_time_info.makeMin = slic_info->hook_time.makeMin;
	le9643_hook_time_info.makeMax = slic_info->hook_time.makeMax;
	le9643_hook_time_info.interDigitMin = slic_info->hook_time.interDigitMin;
	le9643_hook_time_info.flashMin = slic_info->hook_time.flashMin;
	le9643_hook_time_info.flashMax = slic_info->hook_time.flashMax; 
	le9643_hook_time_info.hookMin = slic_info->hook_time.hookMin; 
	
	return 0;
}

int quectel_le9643_get_hook_time(unsigned long ioctl_params)
{
	quectel_slic_info slic_send_info = {0};

	LOGPRINT("Enter quectel_le9643_get_hook_time\n");
	
	slic_send_info.hook_time.breakMin = le9643_hook_time_info.breakMin;
	slic_send_info.hook_time.breakMax = le9643_hook_time_info.breakMax;
	slic_send_info.hook_time.makeMin = le9643_hook_time_info.makeMin;
	slic_send_info.hook_time.makeMax = le9643_hook_time_info.makeMax;
	slic_send_info.hook_time.interDigitMin = le9643_hook_time_info.interDigitMin;
	slic_send_info.hook_time.flashMin = le9643_hook_time_info.flashMin;
	slic_send_info.hook_time.flashMax = le9643_hook_time_info.flashMax;
	slic_send_info.hook_time.hookMin = le9643_hook_time_info.hookMin;

	LOGPRINT("breakMin = %d\n", slic_send_info.hook_time.breakMin);
	LOGPRINT("breakMax = %d\n", slic_send_info.hook_time.breakMax);
	LOGPRINT("makeMin = %d\n", slic_send_info.hook_time.makeMin);
	LOGPRINT("makeMax = %d\n", slic_send_info.hook_time.makeMax);
	LOGPRINT("interDigitMin = %d\n", slic_send_info.hook_time.interDigitMin);
	LOGPRINT("flashMin = %d\n", slic_send_info.hook_time.flashMin);
	LOGPRINT("flashMax = %d\n", slic_send_info.hook_time.flashMax);
	LOGPRINT("hookMin = %d\n", slic_send_info.hook_time.hookMin);
	
	if(copy_to_user((void *) __user ioctl_params, &slic_send_info, sizeof(quectel_slic_info)) != 0)
	{
		LOGPRINT("copy_to_user error\n");
		return -EFAULT;
	}
	return 0;
}

int quectel_le9643_print_register(void)
{
	VpRegisterDump(&devCtx);
	return 0;
}

int quectel_le9643_set_talk_reverse(bool is_reverse)
{
	int error;
	if(hook_status_flag == VP_LINE_EVID_HOOK_OFF) 
	{
		if(is_reverse == true)
		{
			LOGPRINT("VpSetLineState QUECTEL_SLIC_OFFHOOK VP_LINE_TALK_POLREV\n");
			error = VpSetLineState(&lineCtx[0], VP_LINE_TALK_POLREV);
			if(error != VP_STATUS_SUCCESS) {
				LOGPRINT("Error running VpSetLineState on channel 0: %s\n", MapStatus(status));
			}
		}
		else
		{
			LOGPRINT("VpSetLineState QUECTEL_SLIC_OFFHOOK VP_LINE_TALK\n");
			error = VpSetLineState(&lineCtx[0], VP_LINE_TALK);
			if(error != VP_STATUS_SUCCESS) {
				LOGPRINT("Error running VpSetLineState on channel 0: %s\n", MapStatus(status));
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

int quectel_le9643_set_gain(int16 gain_AToD, int16 gain_DToA)
{
	VpStatusType ret; 
	VpOptionAbsGainType op; 
	if(gain_AToD<-6 || gain_AToD>6) return -1;
	if(gain_DToA<-12 || gain_DToA>0) return -1;
	op.gain_AToD = gain_AToD*10;
	op.gain_DToA = gain_DToA*10;
	ret = VpSetOption(VP_NULL, &devCtx, VP_OPTION_ID_ABS_GAIN, &op);
	if(ret != 0)
		return -1;
	LOGPRINT("set_slic_gain AToD=%d DToA=%d rt=%d\n", op.gain_AToD, op.gain_DToA, ret);
	return 0;
}
