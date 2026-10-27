/*
** Copyright (c) 2021 Skyworks Solutions, Inc.
** 2021-11-03 22:44:10
**
** Si3217x ProSLIC API Configuration Tool Version 4.3.1
** Last Updated in API Release: 10.0.1
** source XML file: si3217x_LCQC_LP_constants.xml
**
** Auto generated file from configuration tool.
*/


#include "../inc/proslic.h"
#include "../inc/si3217x.h"

Si3217x_General_Cfg Si3217x_General_Configuration  = {
0x71,                       /* DEVICE_KEY */
BO_DCDC_LCQC_LP,            /* BOM_OPT */
BO_GDRV_NOT_INSTALLED,      /* GDRV_OPTION */
VDC_4P5_16P0,               /* VDC_RANGE_OPT */
VDAA_DISABLED,              /* DAA_ENABLE */
AUTO_ZCAL_ENABLED,          /* ZCAL_ENABLE */
BO_STD_BOM,                 /* PM_BOM */
1850L,                      /* I_OITHRESH (1850mA) */
900L,                       /* I_OITHRESH_LO (900mA) */
1850L,                      /* I_OITHRESH_HI (1850mA) */
128000L,                    /* V_OVTHRESH (128000mV) */
5000L,                      /* V_UVTHRESH (5000mV) */
1000L,                      /* V_UVHYST (1000mV) */
0x00000000L,                /* DCDC_FSW_VTHLO */
0x00000000L,                /* DCDC_FSW_VHYST */
0x0048D15BL,                /* P_TH_HVIC */
0x07FEB800L,                /* COEF_P_HVIC */
0x00083120L,                /* BAT_HYST */
0x03D70A20L,                /* VBATH_EXPECT (60.00V) */
0x070A3D3AL,                /* VBATR_EXPECT (110.00V) */
0x0FFF0000L,                /* PWRSAVE_TIMER */
0x01999A00L,                /* OFFHOOK_THRESH */
0x00F00000L,                /* VBAT_TRACK_MIN */
0x00F00000L,                /* VBAT_TRACK_MIN_RNG */
0x00200000L,                /* DCDC_FSW_NORM */
0x00200000L,                /* DCDC_FSW_NORM_LO */
0x00200000L,                /* DCDC_FSW_RINGING */
0x00200000L,                /* DCDC_FSW_RINGING_LO */
0x0D980000L,                /* DCDC_DIN_LIM */
0x00C00000L,                /* DCDC_VOUT_LIM */
0x0ADD5500L,                /* DCDC_ANA_SCALE */
0x00800000L,                /* THERM_DBI */
0x00FFFFFFL,                /* VOV_DCDC_SLOPE */
0x00A18937L,                /* VOV_DCDC_OS */
0x00A18937L,                /* VOV_RING_BAT_DCDC */
0x00E49BA5L,                /* VOV_RING_BAT_MAX */
0x01018900L,                /* DCDC_VERR */
0x0080C480L,                /* DCDC_VERR_HYST */
0x00400000L,                /* PD_UVLO */
0x00400000L,                /* PD_OVLO */
0x00200000L,                /* PD_OCLO */
0x00400000L,                /* PD_SWDRV */
0x00000000L,                /* DCDC_UVPOL */
0x00200000L,                /* DCDC_RNGTYPE */
0x00300000L,                /* DCDC_ANA_TOFF */
0x00100000L,                /* DCDC_ANA_TONMIN */
0x00FFC000L,                /* DCDC_ANA_TONMAX */
0x50,                       /* IRQEN1 */
0x13,                       /* IRQEN2 */
0x03,                       /* IRQEN3 */
0x00,                       /* IRQEN4 */
0x30,                       /* ENHANCE */
0x00,                       /* DAA_CNTL */
0x3F,                       /* AUTO */
};

Si3217x_audioGain_Cfg Si3217x_audioGain_Presets [] = {
{0x1377080L,0, 0x0L, 0x0L, 0x0L, 0x0L},
{0x80C3180L,0, 0x0L, 0x0L, 0x0L, 0x0L}
};

Si3217x_Ring_Cfg Si3217x_Ring_Presets[] ={
{
/*
	Loop = 500.0 ft @ 0.044 ohms/ft, REN = 3, Rcpe = 600 ohms
	Rprot = 44 ohms, Type = LPR, Waveform = SINE
*/ 
0x00050000L,	/* RTPER */
0x07EFE000L,	/* RINGFR (20.000 Hz) */
0x0025D573L,	/* RINGAMP (64.130 vrms)  */
0x00000000L,	/* RINGPHAS */
0x00000000L,	/* RINGOF (0.000 vdc) */
0x15E5200EL,	/* SLOPE_RING (100.000 ohms) */
0x006C94D6L,	/* IRING_LIM (70.000 mA) */
0x005FDB8EL,	/* RTACTH (52.936 mA) */
0x0FFFFFFFL,	/* RTDCTH (450.000 mA) */
0x00006000L,	/* RTACDB (75.000 ms) */
0x00006000L,	/* RTDCDB (75.000 ms) */
0x0051EB82L,	/* VOV_RING_BAT (5.000 v) */
0x00000000L,	/* VOV_RING_GND (0.000 v) */
0x06974DFAL,	/* VBATR_EXPECT (102.985 v) */
0x80,			/* RINGTALO (2.000 s) */
0x3E,			/* RINGTAHI */
0x00,			/* RINGTILO (4.000 s) */
0x7D,			/* RINGTIHI */
0x00000000L,	/* ADAP_RING_MIN_I */
0x00003000L,	/* COUNTER_IRING_VAL */
0x00051EB8L,	/* COUNTER_VTR_VAL */
0x00000000L,	/* CONST_028 */
0x00000000L,	/* CONST_032 */
0x00000000L,	/* CONST_038 */
0x00000000L,	/* CONST_046 */
0x00000000L,	/* RRD_DELAY */
0x00000000L,	/* RRD_DELAY2 */
0x01893740L,	/* DCDC_VREF_MIN_RNG */
0x58,			/* RINGCON */
0x01,			/* USERSTAT */
0x034BA6FDL,	/* VCM_RING (50.242 v) */
0x034BA6FDL,	/* VCM_RING_FIXED */
0x003126E8L,	/* DELTA_VCM */
0x00200000L,	/* DCDC_RNGTYPE */
},  /* RING_MAX_VBAT_PROVISIONING */
{
/*
	Loop = 500.0 ft @ 0.044 ohms/ft, REN = 5, Rcpe = 600 ohms
	Rprot = 44 ohms, Type = LPR, Waveform = SINE
*/ 
0x00050000L,	/* RTPER */
0x07EFE000L,	/* RINGFR (20.000 Hz) */
0x001BDE11L,	/* RINGAMP (45.000 vrms)  */
0x00000000L,	/* RINGPHAS */
0x00000000L,	/* RINGOF (0.000 vdc) */
0x15E5200EL,	/* SLOPE_RING (100.000 ohms) */
0x006C94D6L,	/* IRING_LIM (70.000 mA) */
0x0068C41CL,	/* RTACTH (57.855 mA) */
0x0FFFFFFFL,	/* RTDCTH (450.000 mA) */
0x00006000L,	/* RTACDB (75.000 ms) */
0x00006000L,	/* RTDCDB (75.000 ms) */
0x0051EB82L,	/* VOV_RING_BAT (5.000 v) */
0x00000000L,	/* VOV_RING_GND (0.000 v) */
0x04F0684FL,	/* VBATR_EXPECT (77.173 v) */
0x80,			/* RINGTALO (2.000 s) */
0x3E,			/* RINGTAHI */
0x00,			/* RINGTILO (4.000 s) */
0x7D,			/* RINGTIHI */
0x00000000L,	/* ADAP_RING_MIN_I */
0x00003000L,	/* COUNTER_IRING_VAL */
0x00051EB8L,	/* COUNTER_VTR_VAL */
0x00000000L,	/* CONST_028 */
0x00000000L,	/* CONST_032 */
0x00000000L,	/* CONST_038 */
0x00000000L,	/* CONST_046 */
0x00000000L,	/* RRD_DELAY */
0x00000000L,	/* RRD_DELAY2 */
0x01893740L,	/* DCDC_VREF_MIN_RNG */
0x58,			/* RINGCON */
0x01,			/* USERSTAT */
0x02783427L,	/* VCM_RING (37.337 v) */
0x02783427L,	/* VCM_RING_FIXED */
0x003126E8L,	/* DELTA_VCM */
0x00200000L,	/* DCDC_RNGTYPE */
},  /* RING_F20_45VRMS_0VDC_LPR */
{
/*
	Loop = 500.0 ft @ 0.044 ohms/ft, REN = 5, Rcpe = 600 ohms
	Rprot = 44 ohms, Type = BALANCED, Waveform = SINE
*/ 
0x00050000L,	/* RTPER */
0x07EFE000L,	/* RINGFR (20.000 Hz) */
0x001BDE11L,	/* RINGAMP (45.000 vrms)  */
0x00000000L,	/* RINGPHAS */
0x00000000L,	/* RINGOF (0.000 vdc) */
0x15E5200EL,	/* SLOPE_RING (100.000 ohms) */
0x006C94D6L,	/* IRING_LIM (70.000 mA) */
0x0068C41CL,	/* RTACTH (57.855 mA) */
0x0FFFFFFFL,	/* RTDCTH (450.000 mA) */
0x00006000L,	/* RTACDB (75.000 ms) */
0x00006000L,	/* RTDCDB (75.000 ms) */
0x0051EB82L,	/* VOV_RING_BAT (5.000 v) */
0x00000000L,	/* VOV_RING_GND (0.000 v) */
0x04F0684FL,	/* VBATR_EXPECT (77.173 v) */
0x80,			/* RINGTALO (2.000 s) */
0x3E,			/* RINGTAHI */
0x00,			/* RINGTILO (4.000 s) */
0x7D,			/* RINGTIHI */
0x00000000L,	/* ADAP_RING_MIN_I */
0x00003000L,	/* COUNTER_IRING_VAL */
0x00051EB8L,	/* COUNTER_VTR_VAL */
0x00000000L,	/* CONST_028 */
0x00000000L,	/* CONST_032 */
0x00000000L,	/* CONST_038 */
0x00000000L,	/* CONST_046 */
0x00000000L,	/* RRD_DELAY */
0x00000000L,	/* RRD_DELAY2 */
0x01893740L,	/* DCDC_VREF_MIN_RNG */
0x58,			/* RINGCON */
0x00,			/* USERSTAT */
0x02783427L,	/* VCM_RING (37.337 v) */
0x02783427L,	/* VCM_RING_FIXED */
0x003126E8L,	/* DELTA_VCM */
0x00200000L,	/* DCDC_RNGTYPE */
}   /* RING_F20_45VRMS_0VDC_BAL */
};

Si3217x_DCfeed_Cfg Si3217x_DCfeed_Presets[] = {
{
0x1C8A024CL,	/* SLOPE_VLIM */
0x1F909679L,	/* SLOPE_RFEED */
0x0040A0E0L,	/* SLOPE_ILIM */
0x1D5B21A9L,	/* SLOPE_DELTA1 */
0x1DD87A3EL,	/* SLOPE_DELTA2 */
0x05A38633L,	/* V_VLIM (48.000 v) */
0x050D2839L,	/* V_RFEED (43.000 v) */
0x03FE7F0FL,	/* V_ILIM  (34.000 v) */
0x00B4F3C3L,	/* CONST_RFEED (15.000 mA) */
0x005D0FA6L,	/* CONST_ILIM (20.000 mA) */
0x002D8D96L,	/* I_VLIM (0.000 mA) */
0x005B0AFBL,	/* LCRONHK (10.000 mA) */
0x006D4060L,	/* LCROFFHK (12.000 mA) */
0x00008000L,	/* LCRDBI (5.000 ms) */
0x0048D595L,	/* LONGHITH (8.000 mA) */
0x003FBAE2L,	/* LONGLOTH (7.000 mA) */
0x00008000L,	/* LONGDBI (5.000 ms) */
0x000F0000L,	/* LCRMASK (150.000 ms) */
0x00080000L,	/* LCRMASK_POLREV (80.000 ms) */
0x00140000L,	/* LCRMASK_STATE (200.000 ms) */
0x00140000L,	/* LCRMASK_LINECAP (200.000 ms) */
0x01BA5E35L,	/* VCM_OH (27.000 v) */
0x0051EB85L,	/* VOV_BAT (5.000 v) */
0x00418937L,	/* VOV_GND (4.000 v) */
},  /* DCFEED_48V_20MA */
{
0x1C8A024CL,	/* SLOPE_VLIM */
0x1EE08C11L,	/* SLOPE_RFEED */
0x0040A0E0L,	/* SLOPE_ILIM */
0x1C940D71L,	/* SLOPE_DELTA1 */
0x1DD87A3EL,	/* SLOPE_DELTA2 */
0x05A38633L,	/* V_VLIM (48.000 v) */
0x050D2839L,	/* V_RFEED (43.000 v) */
0x03FE7F0FL,	/* V_ILIM  (34.000 v) */
0x01241BC9L,	/* CONST_RFEED (15.000 mA) */
0x0074538FL,	/* CONST_ILIM (25.000 mA) */
0x002D8D96L,	/* I_VLIM (0.000 mA) */
0x005B0AFBL,	/* LCRONHK (10.000 mA) */
0x006D4060L,	/* LCROFFHK (12.000 mA) */
0x00008000L,	/* LCRDBI (5.000 ms) */
0x0048D595L,	/* LONGHITH (8.000 mA) */
0x003FBAE2L,	/* LONGLOTH (7.000 mA) */
0x00008000L,	/* LONGDBI (5.000 ms) */
0x000F0000L,	/* LCRMASK (150.000 ms) */
0x00080000L,	/* LCRMASK_POLREV (80.000 ms) */
0x00140000L,	/* LCRMASK_STATE (200.000 ms) */
0x00140000L,	/* LCRMASK_LINECAP (200.000 ms) */
0x01BA5E35L,	/* VCM_OH (27.000 v) */
0x0051EB85L,	/* VOV_BAT (5.000 v) */
0x00418937L,	/* VOV_GND (4.000 v) */
},  /* DCFEED_48V_25MA */
{
0x1E655196L,	/* SLOPE_VLIM */
0x001904EFL,	/* SLOPE_RFEED */
0x0040A0E0L,	/* SLOPE_ILIM */
0x1B4CAD9EL,	/* SLOPE_DELTA1 */
0x1BB0F47CL,	/* SLOPE_DELTA2 */
0x05A38633L,	/* V_VLIM (48.000 v) */
0x043AA4A6L,	/* V_RFEED (36.000 v) */
0x025977EAL,	/* V_ILIM  (20.000 v) */
0x0068B19AL,	/* CONST_RFEED (18.000 mA) */
0x005D0FA6L,	/* CONST_ILIM (20.000 mA) */
0x002D8D96L,	/* I_VLIM (0.000 mA) */
0x005B0AFBL,	/* LCRONHK (10.000 mA) */
0x006D4060L,	/* LCROFFHK (12.000 mA) */
0x00008000L,	/* LCRDBI (5.000 ms) */
0x0048D595L,	/* LONGHITH (8.000 mA) */
0x003FBAE2L,	/* LONGLOTH (7.000 mA) */
0x00008000L,	/* LONGDBI (5.000 ms) */
0x000F0000L,	/* LCRMASK (150.000 ms) */
0x00080000L,	/* LCRMASK_POLREV (80.000 ms) */
0x00140000L,	/* LCRMASK_STATE (200.000 ms) */
0x00140000L,	/* LCRMASK_LINECAP (200.000 ms) */
0x01BA5E35L,	/* VCM_OH (27.000 v) */
0x0051EB85L,	/* VOV_BAT (5.000 v) */
0x00418937L,	/* VOV_GND (4.000 v) */
},  /* DCFEED_PSTN_DET_1 */
{
0x1A10433FL,	/* SLOPE_VLIM */
0x1C206275L,	/* SLOPE_RFEED */
0x0040A0E0L,	/* SLOPE_ILIM */
0x1C1F426FL,	/* SLOPE_DELTA1 */
0x1EB51625L,	/* SLOPE_DELTA2 */
0x041C91DBL,	/* V_VLIM (35.000 v) */
0x03E06C43L,	/* V_RFEED (33.000 v) */
0x038633E0L,	/* V_ILIM  (30.000 v) */
0x022E5DE5L,	/* CONST_RFEED (10.000 mA) */
0x005D0FA6L,	/* CONST_ILIM (20.000 mA) */
0x0021373DL,	/* I_VLIM (0.000 mA) */
0x005B0AFBL,	/* LCRONHK (10.000 mA) */
0x006D4060L,	/* LCROFFHK (12.000 mA) */
0x00008000L,	/* LCRDBI (5.000 ms) */
0x0048D595L,	/* LONGHITH (8.000 mA) */
0x003FBAE2L,	/* LONGLOTH (7.000 mA) */
0x00008000L,	/* LONGDBI (5.000 ms) */
0x000F0000L,	/* LCRMASK (150.000 ms) */
0x00080000L,	/* LCRMASK_POLREV (80.000 ms) */
0x00140000L,	/* LCRMASK_STATE (200.000 ms) */
0x00140000L,	/* LCRMASK_LINECAP (200.000 ms) */
0x01BA5E35L,	/* VCM_OH (27.000 v) */
0x0051EB85L,	/* VOV_BAT (5.000 v) */
0x00418937L,	/* VOV_GND (4.000 v) */
}   /* DCFEED_PSTN_DET_2 */
};

Si3217x_Impedance_Cfg Si3217x_Impedance_Presets[] ={
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=600_0_0 rprot=20 rfuse=24 emi_cap=10*/
{
{0x07F4E380L, 0x000D8F00L, 0x0000A980L, 0x1FFD5200L,    /* TXACEQ */
 0x07F2DB00L, 0x0013B280L, 0x1FFE3C00L, 0x1FFCC900L},   /* RXACEQ */
{0x0012DC80L, 0x1FED3780L, 0x019C0300L, 0x00BF5C80L,    /* ECFIR/ECIIR */
 0x01ACD900L, 0x1F73A980L, 0x00468880L, 0x1FEA7580L,
 0x1FFC1180L, 0x00034800L, 0x0F60F800L, 0x189DA600L},
{0x007DD200L, 0x1F053480L, 0x007CFC00L, 0x0FF66E00L,    /* ZSYNTH */
 0x18098F80L, 0x5D}, 
 0x08C4D280L,   /* TXACGAIN */
 0x014D1380L,   /* RXACGAIN */
 0x07A0BF00L, 0x185F4180L, 0x07417E00L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 },  /* ZSYN_600_0_0_30_0 */
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=270_750_150 rprot=20 rfuse=24 emi_cap=10*/
{
{0x07371080L, 0x1FD14680L, 0x000BA300L, 0x1FFDBE00L,    /* TXACEQ */
 0x0A85C380L, 0x1BA12D80L, 0x00803380L, 0x1FDB9F00L},   /* RXACEQ */
{0x001BB880L, 0x1FB47500L, 0x01BD1F80L, 0x006BDB00L,    /* ECFIR/ECIIR */
 0x02CBE700L, 0x0030B900L, 0x00CD9E00L, 0x00403980L,
 0x0024D800L, 0x1FD90480L, 0x0CA30B00L, 0x1B548F00L},
{0x1F48AA80L, 0x00D70100L, 0x1FE02280L, 0x0D974E00L,    /* ZSYNTH */
 0x1A67BD80L, 0xB3}, 
 0x08000000L,   /* TXACGAIN */
 0x010D9C00L,   /* RXACGAIN */
 0x07BBA700L, 0x18445980L, 0x07774E00L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 },  /* ZSYN_270_750_150_30_0 */
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=370_620_310 rprot=20 rfuse=24 emi_cap=10*/
{
{0x08190380L, 0x1FBFA080L, 0x1FFCBF80L, 0x1FFCDE80L,    /* TXACEQ */
 0x0A01C380L, 0x1C010200L, 0x1F99D400L, 0x1FDF8D00L},   /* RXACEQ */
{0x002D2D00L, 0x1F5F7480L, 0x029E7C80L, 0x1ED21180L,    /* ECFIR/ECIIR */
 0x041E4A00L, 0x1E9C1B80L, 0x0182A700L, 0x000EE500L,
 0x003FF680L, 0x1FBEFD00L, 0x0D9E8380L, 0x1A5B9880L},
{0x00215C00L, 0x1F936780L, 0x004B2C00L, 0x0F109400L,    /* ZSYNTH */
 0x18EEF680L, 0x95}, 
 0x08000000L,   /* TXACGAIN */
 0x012D8900L,   /* RXACGAIN */
 0x07B51500L, 0x184AEB80L, 0x076A2A80L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 },  /* ZSYN_370_620_310_30_0 */
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=220_820_120 rprot=20 rfuse=24 emi_cap=10*/
{
{0x0700AB00L, 0x1FCD6D00L, 0x0004FF00L, 0x1FFC1480L,    /* TXACEQ */
 0x0A7E6780L, 0x1BBA9400L, 0x009D1700L, 0x1FD55400L},   /* RXACEQ */
{0x00152D80L, 0x1FD2B780L, 0x01719200L, 0x00FCF480L,    /* ECFIR/ECIIR */
 0x0251C000L, 0x00C01980L, 0x0086CB00L, 0x0055D880L,
 0x001C4280L, 0x1FE0BF80L, 0x0C7D1700L, 0x1B7A2080L},
{0x1F80B980L, 0x00222B00L, 0x005D0600L, 0x0C71D100L,    /* ZSYNTH */
 0x1B8D7D80L, 0xAE}, 
 0x08000000L,   /* TXACGAIN */
 0x01056600L,   /* RXACGAIN */
 0x07BC4080L, 0x1843C000L, 0x07788100L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 },  /* ZSYN_220_820_120_30_0 */
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=600_0_0 rprot=20 rfuse=24 emi_cap=10*/
{
{0x07F4E380L, 0x000D8F00L, 0x0000A980L, 0x1FFD5200L,    /* TXACEQ */
 0x07F2DB00L, 0x0013B280L, 0x1FFE3C00L, 0x1FFCC900L},   /* RXACEQ */
{0x0012DC80L, 0x1FED3780L, 0x019C0300L, 0x00BF5C80L,    /* ECFIR/ECIIR */
 0x01ACD900L, 0x1F73A980L, 0x00468880L, 0x1FEA7580L,
 0x1FFC1180L, 0x00034800L, 0x0F60F800L, 0x189DA600L},
{0x007DD200L, 0x1F053480L, 0x007CFC00L, 0x0FF66E00L,    /* ZSYNTH */
 0x18098F80L, 0x5D}, 
 0x08C4D280L,   /* TXACGAIN */
 0x014D1380L,   /* RXACGAIN */
 0x07A0BF00L, 0x185F4180L, 0x07417E00L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 },  /* ZSYN_600_0_1000_30_0 */
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=200_680_100 rprot=20 rfuse=24 emi_cap=10*/
{
{0x0760DB80L, 0x1FBEE000L, 0x00020480L, 0x1FFC7680L,    /* TXACEQ */
 0x09C77900L, 0x1D163B80L, 0x00737880L, 0x1FDEAF80L},   /* RXACEQ */
{0x1FF54280L, 0x0054E900L, 0x00803880L, 0x01EB3980L,    /* ECFIR/ECIIR */
 0x01FA9180L, 0x1FA5C980L, 0x01C1DE00L, 0x1F038080L,
 0x00929B80L, 0x1F689100L, 0x06771000L, 0x0170BB80L},
{0x01AC1900L, 0x1BC8C680L, 0x028ADD00L, 0x0A133300L,    /* ZSYNTH */
 0x1DEA3880L, 0x8D}, 
 0x08000000L,   /* TXACGAIN */
 0x01122180L,   /* RXACGAIN */
 0x07BAB880L, 0x18454800L, 0x07757100L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 },  /* ZSYN_200_680_100_30_0 */
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=220_820_115 rprot=20 rfuse=24 emi_cap=10*/
{
{0x06F4A300L, 0x1FD46380L, 0x00097A80L, 0x1FFCF980L,    /* TXACEQ */
 0x0A634A00L, 0x1BF39280L, 0x009B8E80L, 0x1FD63480L},   /* RXACEQ */
{0x001B2200L, 0x1FDE9200L, 0x01159380L, 0x01EBF600L,    /* ECFIR/ECIIR */
 0x01098D00L, 0x02005380L, 0x1FBE9500L, 0x0097BC00L,
 0x0012D280L, 0x1FEB4A00L, 0x0CBE2100L, 0x1B2F2A80L},
{0x1FE37780L, 0x1EC55F80L, 0x0156AB00L, 0x0A138100L,    /* ZSYNTH */
 0x1DEA2380L, 0xB0}, 
 0x08000000L,   /* TXACGAIN */
 0x01043080L,   /* RXACGAIN */
 0x07BB9F00L, 0x18446180L, 0x07773E00L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 },  /* ZSYN_220_820_115_30_0 */
/* Source: Database file: cwdb.db */
/* Database information: */
/* parameters: zref=600_0_0 rprot=20 rfuse=24 emi_cap=0*/
{
{0x08141E00L, 0x1FE78180L, 0x0006C800L, 0x1FFA6380L,    /* TXACEQ */
 0x07FB7380L, 0x1FED4280L, 0x0001AA80L, 0x1FF01E80L},   /* RXACEQ */
{0x0053F300L, 0x1F458080L, 0x02957900L, 0x000E3780L,    /* ECFIR/ECIIR */
 0x01973F00L, 0x000DE980L, 0x1FC33E00L, 0x00074D80L,
 0x0021CB80L, 0x1FE8A180L, 0x1FC89C00L, 0x1FCE9380L},
{0x00213E00L, 0x1FC33180L, 0x001BA200L, 0x0E24EA00L,    /* ZSYNTH */
 0x19DADA80L, 0x72}, 
 0x08A26F80L,   /* TXACGAIN */
 0x0144A000L,   /* RXACGAIN */
 0x07BEB280L, 0x18414E00L, 0x077D6500L,    /* RXACHPF */
#ifdef ENABLE_HIRES_GAIN
 0, 0  /* TXGAIN*10, RXGAIN*10 (hi_res) */
#else
 0, 0  /* TXGAIN, RXGAIN */
#endif
 }   /* WB_ZSYN_600_0_0_20_0 */
};

Si3217x_FSK_Cfg Si3217x_FSK_Presets[] ={
{
{
0x02232000L,	 /* FSK01 */
0x077C2000L 	 /* FSK10 */
},
{
0x0015C000L,	 /* FSKAMP0 (0.080 vrms )*/
0x000BA000L 	 /* FSKAMP1 (0.080 vrms) */
},
{
0x06B60000L,	 /* FSKFREQ0 (2200.0 Hz space) */
0x079C0000L 	 /* FSKFREQ1 (1200.0 Hz mark) */
},
0x00,			 /* FSK8 */
0x00,			 /* FSKDEPTH (1 deep fifo) */
},  /* DEFAULT_FSK */
{
{
0x026E4000L,	 /* FSK01 */
0x0694C000L 	 /* FSK10 */
},
{
0x0014C000L,	 /* FSKAMP0 (0.080 vrms )*/
0x000CA000L 	 /* FSKAMP1 (0.080 vrms) */
},
{
0x06D20000L,	 /* FSKFREQ0 (2100.0 Hz space) */
0x078B0000L 	 /* FSKFREQ1 (1300.0 Hz mark) */
},
0x00,			 /* FSK8 */
0x00,			 /* FSKDEPTH (1 deep fifo) */
}   /* ETSI_FSK */
};

Si3217x_PulseMeter_Cfg Si3217x_PulseMeter_Presets[] ={
{
0x007A2B6AL,  /* PM_AMP_THRESH (1.000) */
0,            /* Freq (12kHz) */ 
0,            /* PM_RAMP (24kHz)*/
0,            /* PM_FORCE (First)*/
0,            /* PWR_SAVE (off)*/
0,            /* PM_AUTO (off)*/
0x07D00000L,  /* PM_active (2000 ms) */
0x07D00000L   /* PM_inactive (2000 ms) */
 }   /* DEFAULT_PULSE_METERING */
};

Si3217x_Tone_Cfg Si3217x_Tone_Presets[] = {
{
	{
	0x07B30000L,	 /* OSC1FREQ (350.000 Hz) */
	0x000C6000L,	 /* OSC1AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x00,			 /* O1TALO (0 ms) */
	0x00,			 /* O1TAHI */
	0x00,			 /* O1TILO (0 ms) */
	0x00			 /* O1TIHI */
	},
	{
	0x07870000L,	 /* OSC2FREQ (440.000 Hz) */
	0x000FA000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x00,			 /* O2TALO (0 ms) */
	0x00,			 /* O2TAHI */
	0x00,			 /* O2TILO (0 ms) */
	0x00 			 /* O2TIHI */
	},
	0x66 			 /* OMODE */
},  /* TONEGEN_FCC_DIAL */
{
	{
	0x07700000L,	 /* OSC1FREQ (480.000 Hz) */
	0x00112000L,	 /* OSC1AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0xA0,			 /* O1TALO (500 ms) */
	0x0F,			 /* O1TAHI */
	0xA0,			 /* O1TILO (500 ms) */
	0x0F			 /* O1TIHI */
	},
	{
	0x07120000L,	 /* OSC2FREQ (620.000 Hz) */
	0x00164000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0xA0,			 /* O2TALO (500 ms) */
	0x0F,			 /* O2TAHI */
	0xA0,			 /* O2TILO (500 ms) */
	0x0F 			 /* O2TIHI */
	},
	0x66 			 /* OMODE */
},  /* TONEGEN_FCC_BUSY */
{
	{
	0x07700000L,	 /* OSC1FREQ (480.000 Hz) */
	0x00112000L,	 /* OSC1AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x80,			 /* O1TALO (2000 ms) */
	0x3E,			 /* O1TAHI */
	0x00,			 /* O1TILO (4000 ms) */
	0x7D			 /* O1TIHI */
	},
	{
	0x07870000L,	 /* OSC2FREQ (440.000 Hz) */
	0x000FA000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x80,			 /* O2TALO (2000 ms) */
	0x3E,			 /* O2TAHI */
	0x00,			 /* O2TILO (4000 ms) */
	0x7D 			 /* O2TIHI */
	},
	0x66 			 /* OMODE */
},  /* TONEGEN_FCC_RINGBACK */
{
	{
	0x07700000L,	 /* OSC1FREQ (480.000 Hz) */
	0x00112000L,	 /* OSC1AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x60,			 /* O1TALO (300 ms) */
	0x09,			 /* O1TAHI */
	0x60,			 /* O1TILO (300 ms) */
	0x09			 /* O1TIHI */
	},
	{
	0x07120000L,	 /* OSC2FREQ (620.000 Hz) */
	0x00164000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x60,			 /* O2TALO (300 ms) */
	0x09,			 /* O2TAHI */
	0x40,			 /* O2TILO (200 ms) */
	0x06 			 /* O2TIHI */
	},
	0x66 			 /* OMODE */
},  /* TONEGEN_FCC_REORDER */
{
	{
	0x07700000L,	 /* OSC1FREQ (480.000 Hz) */
	0x00112000L,	 /* OSC1AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x40,			 /* O1TALO (200 ms) */
	0x06,			 /* O1TAHI */
	0x40,			 /* O1TILO (200 ms) */
	0x06			 /* O1TIHI */
	},
	{
	0x07120000L,	 /* OSC2FREQ (620.000 Hz) */
	0x00164000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x40,			 /* O2TALO (200 ms) */
	0x06,			 /* O2TAHI */
	0x40,			 /* O2TILO (200 ms) */
	0x06 			 /* O2TIHI */
	},
	0x66 			 /* OMODE */
},  /* TONEGEN_FCC_CONGESTION */
{
	{
	0x1F2F0000L,	 /* OSC1FREQ (2130.000 Hz) */
	0x0063A000L,	 /* OSC1AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x80,			 /* O1TALO (80 ms) */
	0x02,			 /* O1TAHI */
	0x80,			 /* O1TILO (80 ms) */
	0x02			 /* O1TIHI */
	},
	{
	0x1B8E0000L,	 /* OSC2FREQ (2750.000 Hz) */
	0x00A84000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x80,			 /* O2TALO (80 ms) */
	0x02,			 /* O2TAHI */
	0x40,			 /* O2TILO (1000 ms) */
	0x1F 			 /* O2TIHI */
	},
	0x66 			 /* OMODE */
},  /* TONEGEN_FCC_CAS */
{
	{
	0x07870000L,	 /* OSC1FREQ (440.000 Hz) */
	0x000FA000L,	 /* OSC1AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x60,			 /* O1TALO (300 ms) */
	0x09,			 /* O1TAHI */
	0x00,			 /* O1TILO (8000 ms) */
	0xFA			 /* O1TIHI */
	},
	{
	0x1B8E0000L,	 /* OSC2FREQ (2750.000 Hz) */
	0x00A84000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x80,			 /* O2TALO (2000 ms) */
	0x3E,			 /* O2TAHI */
	0x00,			 /* O2TILO (4000 ms) */
	0x7D 			 /* O2TIHI */
	},
	0x06 			 /* OMODE */
},  /* TONEGEN_FCC_SAS */
{
	{
	0x1F2F0000L,	 /* OSC1FREQ (2130.000 Hz) */
	0x01BD0000L,	 /* OSC1AMP (-5.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x20,			 /* O1TALO (100 ms) */
	0x03,			 /* O1TAHI */
	0x20,			 /* O1TILO (100 ms) */
	0x03			 /* O1TIHI */
	},
	{
	0x1B8E0000L,	 /* OSC2FREQ (2750.000 Hz) */
	0x02EFC000L,	 /* OSC2AMP (-5.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x20,			 /* O2TALO (100 ms) */
	0x03,			 /* O2TAHI */
	0x20,			 /* O2TILO (100 ms) */
	0x03 			 /* O2TIHI */
	},
	0x66 			 /* OMODE */
},  /* TONEGEN_ETSI_DTAS */
{
	{
	0x05A40000L,	 /* OSC1FREQ (1004.000 Hz) */
	0x005DE000L,	 /* OSC1AMP (-10.000 dBm) */
	0x00000000L,	 /* OSC1PHAS (0.000 rad) */
	0x00,			 /* O1TALO (8000 ms) */
	0xFA,			 /* O1TAHI */
	0x00,			 /* O1TILO (8000 ms) */
	0xFA			 /* O1TIHI */
	},
	{
	0x07870000L,	 /* OSC2FREQ (440.000 Hz) */
	0x000FA000L,	 /* OSC2AMP (-18.000 dBm) */
	0x00000000L,	 /* OSC2PHAS (0.000 rad) */
	0x80,			 /* O2TALO (2000 ms) */
	0x3E,			 /* O2TAHI */
	0x00,			 /* O2TILO (4000 ms) */
	0x7D 			 /* O2TIHI */
	},
	0x47 			 /* OMODE */
}   /* TONEGEN_1004 */
};

Si3217x_PCM_Cfg Si3217x_PCM_Presets[] ={
	{
	0x01, 	 /* PCM_FMT - u-Law */
	0x00, 	 /* WIDEBAND - DISABLED (3.4kHz BW) */
	0x00, 	 /* PCM_TRI - PCLK RISING EDGE */
	0x00, 	 /* TX_EDGE - PCLK RISING EDGE */
	0x00 	 /* A-LAW -  INVERT NONE */
	},  /* PCM_8ULAW */
	{
	0x00, 	 /* PCM_FMT - A-Law */
	0x00, 	 /* WIDEBAND - DISABLED (3.4kHz BW) */
	0x00, 	 /* PCM_TRI - PCLK RISING EDGE */
	0x00, 	 /* TX_EDGE - PCLK RISING EDGE */
	0x00 	 /* A-LAW -  INVERT NONE */
	},  /* PCM_8ALAW */
	{
	0x03, 	 /* PCM_FMT - 16-bit Linear */
	0x00, 	 /* WIDEBAND - DISABLED (3.4kHz BW) */
	0x00, 	 /* PCM_TRI - PCLK RISING EDGE */
	0x00, 	 /* TX_EDGE - PCLK RISING EDGE */
	0x00 	 /* A-LAW -  INVERT NONE */
	},  /* PCM_16LIN */
	{
	0x03, 	 /* PCM_FMT - 16-bit Linear */
	0x01, 	 /* WIDEBAND - ENABLED (7kHz BW) */
	0x00, 	 /* PCM_TRI - PCLK RISING EDGE */
	0x00, 	 /* TX_EDGE - PCLK RISING EDGE */
	0x00 	 /* A-LAW -  INVERT NONE */
	}   /* PCM_16LIN_WB */
};

