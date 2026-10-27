/**
 * @file quectel_slic_app.c
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
  2021/04/29	   ricaro      Create file and add SLIC basic API
  ============================================================================*/

#include "proslic/proslic_sys.h"

#define QUECTEL_SLIC_SUCCESS  0
int quectel_slic_id = 0;
typedef enum 
{
    QL_SLIC_AUTO,
    QL_SLIC_LE9643,
    QL_SLIC_SI321XX,
    QL_SLIC_TYPE_MAX,
}QL_SLIC_TYPE_E;

unsigned char quectel_slic_init(quectel_slic_info *slic_info)
{
    int error = -1;
    int type = 0;

    type = slic_info->slic_type;
    if((type == QL_SLIC_AUTO) || (type == QL_SLIC_LE9643)) {
        error = quectel_le9643_init(slic_info->country_code);
        if(!error) {
            quectel_slic_id = QUECTEL_SLIC_LE9643;
            return 0;
        }
    }
    if((type == QL_SLIC_AUTO) || (type == QL_SLIC_SI321XX)) {
        error = quectel_si321xx_init(slic_info->country_code);
        if(!error) {
            quectel_slic_id = QUECTEL_SLIC_SI321XX;
            return 0;
        }
    }
    return error;
}

int quectel_slic_get_event(unsigned long ioctl_params)
{
	if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        quectel_le9643_get_event(ioctl_params);
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        quectel_si321xx_get_event(ioctl_params);
    }
    return 0;
}

int quectel_slic_set_tone(quectel_slic_info *slic_info)
{
	if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        quectel_le9643_set_tone(slic_info);
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        quectel_si321xx_set_tone(slic_info);
    }
    return 0;
}

int quectel_slic_set_cid(quectel_slic_info *slic_info)
{
	if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        if(slic_info->call_cid_mode == QL_CALL_CID_DTMF)
        {
            quectel_le9643_set_dtmf_cid(slic_info);
        }
        else
        {
            quectel_le9643_set_fsk_cid(slic_info);
        }
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        if(slic_info->call_cid_mode == QL_CALL_CID_DTMF)
        {
            quectel_si321xx_set_dtmf_cid(slic_info);
        }
        else
        {
            quectel_si321xx_set_fsk_cid(slic_info);
        }
    }
    return 0;
}

int quectel_slic_set_ring(quectel_slic_info *slic_info)
{
	if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        quectel_le9643_set_ring(slic_info);
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        quectel_si321xx_set_ring(slic_info);
    }
    return 0;
}

int quectel_slic_close_dev(void)
{
    if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        quectel_le9643_close_dev();
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        quectel_si321xx_close_dev();
    }
    return 0;
}

int quectel_slic_set_hook_time(quectel_slic_info *slic_info)
{
    if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        quectel_le9643_set_hook_time(slic_info);
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        quectel_si321xx_set_hook_time(slic_info);
    }
    return 0;
}

int quectel_slic_get_hook_time(unsigned long ioctl_params)
{
    if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        quectel_le9643_get_hook_time(ioctl_params);
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        quectel_si321xx_get_hook_time(ioctl_params);
    }
    return 0;
}

int quectel_slic_print_register(void)
{
    if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        quectel_le9643_print_register();
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        quectel_si321xx_print_register();
    }
    return 0;
}

int quectel_slic_set_talk_reverse(quectel_slic_info *slic_info)
{
    int ret;
    if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        ret = quectel_le9643_set_talk_reverse(slic_info->is_reverse);
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        ret = quectel_si321xx_set_talk_reverse(slic_info->is_reverse);
    }
    return ret;
}

int quectel_slic_set_gain(quectel_slic_info *slic_info)
{
    int ret;
    if(quectel_slic_id == QUECTEL_SLIC_LE9643) {
        ret = quectel_le9643_set_gain(slic_info->gain_AToD, slic_info->gain_DToA);
    } else if((quectel_slic_id == QUECTEL_SLIC_SI321XX)){
        ret = quectel_si321xx_set_gain(slic_info->gain_AToD, slic_info->gain_DToA);
    }
    return ret;
}