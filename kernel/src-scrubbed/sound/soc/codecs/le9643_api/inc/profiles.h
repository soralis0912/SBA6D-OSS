/*
 * profiles.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:   U:\dbuckley\ztap\trunk\applications\quickstarts\sdk_examples\Le9641_DTMF_CID\profiles.vpw
 *   Type:   miSLIC- Le9641 - Buck-Boost Fixed Ringing
 *   Date:   Thursday, April 21, 2016 16:06:25
 *   Device: miSLIC Le9641
 *
 *   This file was generated with Profile Wizard Version: P2.8.0
 */

#ifndef PROFILES_H
#define PROFILES_H

#ifdef VP_API_TYPES_H
#include "vp_api_types.h"
#else
typedef unsigned char VpProfileDataType;
#endif


/************** Device Profile **************/
extern const VpProfileDataType DEV_PROFILE_Buck_Boost[];/* Device Configuration Data - 5V 22uH 1.5A Buck-Boost */
extern const VpProfileDataType miSLIC_IB_DEV_PROFILE[];/* Device Configuration Data - 5V  Inverting Boost */
extern const VpProfileDataType DEV_PROFILE_Buck_Boost_3V[];/* Device Configuration Data - 3.3V Buck-Boost */
extern const VpProfileDataType DEV_PROFILE_PL_Buck_Boost_zm[];/* Device Configuration Data - zm custom Buck-Boost */

/************** DC Profile **************/
extern const VpProfileDataType DC_FXS_miSLIC_BB_DEF[];/* DC FXS Default - Use for all countries unless country file exists - 25mA Current Feed */

/************** AC Profile **************/
extern const VpProfileDataType AC_FXS_RF14_600R_DEF[];/* AC FXS RF14 600R Normal Coefficients (Default)  */
extern const VpProfileDataType miSLIC_AC_600[];
extern const VpProfileDataType AC_FXS_RF14_600_ABS[];

/************** Ringing Profile **************/
extern const VpProfileDataType RING_miSLIC_BB_DEF[]; /* Default Ringing 25Hz 50Vrms Tracking, AC Trip - Use for all countries unless country profile exists */
extern const VpProfileDataType miSLIC_BB_RING_25HZ_SINE[]; /* China Ringing 25Hz 60Vrms Fixed, AC Trip */
extern const VpProfileDataType miSLIC_BB_RING_25HZ_Low[]; /* China Ringing 25Hz 60Vpk Fixed, AC Trip */
extern const VpProfileDataType RING_miSLIC_BB_DE[];
/************** Tone Profile **************/

/************** Cadence Profile **************/
extern const VpProfileDataType CR_CN[];              /* China Ringing Signal Cadence */
extern const VpProfileDataType CR_DE[];              /* Germany Ringing Signal Cadence */

/************** Caller ID Profile **************/
extern const VpProfileDataType CID_TYPE1_CN_DTMF[];           /* DTMF CID Type1 */
extern const VpProfileDataType CID_TYPE2_CN_DTMF[];
extern const VpProfileDataType CID_TYPE2_CN_DTMF_NoACK[];     //just for test

extern const VpProfileDataType CID_TYPE1_CN_FSK[];
extern const VpProfileDataType CID_TYPE2_CN_FSK[];
extern const VpProfileDataType CID_TYPE2_CN_FSK_NoACK[];      //just for test

extern const VpProfileDataType CID_TYPE1_DE_FSK[];
extern const VpProfileDataType CID_TYPE2_DE_FSK[];
extern const VpProfileDataType CID_TYPE2_DE_FSK_NoACK[];
/************** Metering_Profile **************/

extern const VpProfileDataType DTMF_standard_tone[][20];
extern const VpProfileDataType DTMF_standard_cadence[][20];

extern const VpProfileDataType CN_Chinese_standard_tone[][20];
extern const VpProfileDataType CN_Chinese_standard_cadence[][40];

extern const VpProfileDataType DE_Germany_standard_tone[][20];
extern const VpProfileDataType DE_Germany_standard_cadence[][40];

extern const VpProfileDataType ES_Spain_standard_tone[][20];
extern const VpProfileDataType ES_Spain_standard_cadence[][40];

extern const VpProfileDataType UK_United_Kingdom_standard_tone[][20];
extern const VpProfileDataType UK_United_Kingdom_standard_cadence[][40];

extern const VpProfileDataType BR_Brazil_standard_tone[][20];
extern const VpProfileDataType BR_Brazil_standard_cadence[][40];

extern const VpProfileDataType FR_France_standard_tone[][20];
extern const VpProfileDataType FR_France_standard_cadence[][40];

extern const VpProfileDataType QA_Qatar_standard_tone[][20];
extern const VpProfileDataType QA_Qatar_standard_cadence[][40];
#endif /* PROFILES_H */

