/*
 * profiles.c --
 *
 * This file contains profile data in byte format
 *
 * Project Info --
 *   File:   U:\dbuckley\ztap\trunk\applications\quickstarts\sdk_examples\Le9641_DTMF_CID\profiles.vpw
 *   Type:   miSLIC- Le9641 - Buck-Boost Fixed Ringing
 *   Date:   Thursday, April 21, 2016 16:06:25
 *   Device: miSLIC Le9641
 *
 *   This file was generated with Profile Wizard Version: P2.8.0
 */

#include "../inc/profiles.h"


/************** Device Profile **************/
// le9641
/* Device Configuration Data - 5V 22uH 1.5A Buck-Boost */
const VpProfileDataType DEV_PROFILE_Buck_Boost[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0xFF, 0x00,       /* Device Profile for Le9641 device */
   0x31,                   /* Profile length = 49 + 4 = 53 bytes */
   0x07,                   /* Profile format version = 7 */
   0x14,                   /* MPI section length = 20 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0x46, 0x02,             /* PCLK = 8.192 MHz; INTM = CMOS-compatible */
   //0x44, 0x06,             /* PCM Clock Slot = 6 TX / 0 RX; XE = Neg. */
   0x44, 0x40,             /* PCM Clock Slot = 6 TX / 0 RX; XE = Neg. */
   0x5E, 0x14, 0x00,       /* Device Mode Register */
   0xF6, 0x95, 0x00, 0x94, /* Switching Regulator Timing Params */
         0x40, 0xE4, 0x40,
   0xE4, 0x44, 0x8F, 0x0A, /* Switching Regulator Params */
   0xE6, 0x60,             /* Switching Regulator Control */
/* Formatted Parameters Section -------------------------------------------- */
   0x00,                   /* IO21 Mode: Digital */
                           /* IO22 Mode: Digital */
   0x40,                   /* Dial pulse correction: 4 ms */
                           /* Switcher Configuration =
                              BuckBoost 22uH 1.5A miSLIC (5V in, 100 V out) */
   0x00, 0x00,             /* Over-current event threshold = Off */
   0x01,                   /* Leading edge blanking window = 81 ns */
   0x30, 0x14, 0x30, 0x14, /* FreeRun Timing Parameters */
         0x30, 0x14,
   0xFF,
   0x95, 0x00,             /* Low-Power Timing Parameters */
   0x78, 0x78,             /* Switcher Limit Voltages 120 V, 120 V */
   0x44,                   /* Charge pump disabled */
                           /* Charge Pump Overload Protection: Disabled */
   0x3C,                   /* Switcher Input 12 V */
   0x00, 0x00, 0x00, 0x00, /* Battery Backup Timing Parameters */
         0x00, 0x00,
   0x00, 0x00, 0x00        /* Soft Startup Timing (Disabled) */
};

/************** Device Profile **************/
// le9643
/* Device Configuration Inverting Boost  */
const VpProfileDataType miSLIC_IB_DEV_PROFILE[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0xFF, 0x00,       /* Device Profile for Le9661 device */
   0x33,                   /* Profile length = 51 + 4 = 55 bytes */
   0x08,                   /* Profile format version = 8 */
   0x14,                   /* MPI section length = 20 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0x46, 0x02,             /* PCLK = 2.048 MHz; INTM = CMOS-compatible */
   0x44, 0x40,             /* PCM Clock Slot = 0 TX / 0 RX; XE = Pos. */
   0x5E, 0x04, 0x04,       /* Device Mode Register */
   0xF6, 0x96, 0x00, 0x94, /* Switching Regulator Timing Params */
         0x52, 0xD0, 0x52,
   0xE4, 0x04, 0x92, 0x0A, /* Switching Regulator Params */
   0xE6, 0x00,             /* Switching Regulator Control */
/* Formatted Parameters Section -------------------------------------------- */
   0x00,                   /* IO21 Mode: Digital */
                           /* IO22 Mode: Digital */
   0x03,                   /* Dial pulse correction: 0 ms */
                           /* Switcher Configuration =
                              Inverting Boost (12 V in, 90 V out) */
   0x00, 0x00,             /* Over-current event threshold = Off */
   0x01,                   /* Leading edge blanking window = 81 ns */
   0x65, 0x00, 0x64, 0x52, /* FreeRun Timing Parameters */
         0x64, 0x52,
   0xFF,
   0x97, 0xFF,             /* Low-Power Timing Parameters */
   0x58, 0x58,             /* Switcher Limit Voltages 88 V, 88 V */
   0x05,                   /* Charge pump enabled */
                           /* Charge Pump Overload Protection: Disabled */
   0x3C,                   /* Switcher Input 12 V */
   0x00, 0x00, 0x00, 0x00, /* Battery Backup Timing Parameters */
         0x00, 0x00,
   0x00, 0x00, 0x00,       /* Soft Startup Timing (Disabled) */
   0x00, 0x00              /* SwCtrl for Free Run and Battery Backup */

};

/* Device Configuration Data - 3V 15uH 2A Buck-Boost */
const VpProfileDataType DEV_PROFILE_Buck_Boost_3V[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0xFF, 0x00,       /* Device Profile for Le9643 device */
   0x33,                   /* Profile length = 51 + 4 = 55 bytes */
   0x08,                   /* Profile format version = 8 */
   0x14,                   /* MPI section length = 20 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0x46, 0x02,             /* PCLK = 2.048 MHz; INTM = CMOS-compatible */
   0x44, 0x40,             /* PCM Clock Slot = 0 TX / 0 RX; XE = Pos. */
   0x5E, 0x14, 0x00,       /* Device Mode Register */
   0xF6, 0x95, 0x00, 0xB0, /* Switching Regulator Timing Params */
         0x40, 0xE4, 0x40,
   0xE4, 0x44, 0x8F, 0x0A, /* Switching Regulator Params */
   0xE6, 0x60,             /* Switching Regulator Control */
/* Formatted Parameters Section -------------------------------------------- */
   0x00,                   /* IO21 Mode: Digital */
                           /* IO22 Mode: Digital */
   0x40,                   /* Dial pulse correction: 4 ms */
                           /* Switcher Configuration =
                              BuckBoost 15uH 2A miSLIC (3V3 in, 100 V out) */
   0x00, 0x00,             /* Over-current event threshold = Off */
   0x01,                   /* Leading edge blanking window = 81 ns */
   0x30, 0x31, 0x64, 0x31, /* FreeRun Timing Parameters */
         0x64, 0x31,
   0xFF,
   0x95, 0x00,             /* Low-Power Timing Parameters */
   0x62, 0x62,             /* Switcher Limit Voltages 98 V, 98 V */
   0x04,                   /* Charge pump disabled */
                           /* Charge Pump Overload Protection: Disabled */
   0x3C,                   /* Switcher Input 12 V */
   0x00, 0x00, 0x00, 0x00, /* Battery Backup Timing Parameters */
         0x00, 0x00,
   0x29, 0x00, 0x14,       /* Soft Startup Timing (100 ms) */
   0x00, 0x00              /* SwCtrl for Free Run and Battery Backup */

};

/* Device Configuration Data Power Limted - 47uH 1.2A Buck-Boost */
const VpProfileDataType DEV_PROFILE_PL_Buck_Boost_zm[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0xFF, 0x00,       /* Device Profile for Le9643 device */
   0x33,                   /* Profile length = 51 + 4 = 55 bytes */
   0x08,                   /* Profile format version = 8 */
   0x14,                   /* MPI section length = 20 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0x46, 0x02,             /* PCLK = 2.048 MHz; INTM = CMOS-compatible */
   0x44, 0x40,             /* PCM Clock Slot = 0 TX / 0 RX; XE = Pos. */
   0x5E, 0x14, 0x00,       /* Device Mode Register */
   0xF6, 0x95, 0x00, 0x58, /* Switching Regulator Timing Params */
         0x30, 0x64, 0x30,
   0xE4, 0x04, 0x8F, 0x0A, /* Switching Regulator Params */
   0xE6, 0x60,             /* Switching Regulator Control */
/* Formatted Parameters Section -------------------------------------------- */
   0x01,                   /* IO21 Mode: Digital with Interrupt */
                           /* IO22 Mode: Digital */
   0xA0,                   /* Dial pulse correction: 10 ms */
                           /* Switcher Configuration =
                              BuckBoost 47uH 1.5A miSLIC Power Limited (12 V in, 100 V out) */
   0x00, 0x00,             /* Over-current event threshold = Off */
   0x01,                   /* Leading edge blanking window = 81 ns */
   0x30, 0x14, 0x30, 0x14, /* FreeRun Timing Parameters */
         0x30, 0x14,
   0xFF,
   0x95, 0x00,             /* Low-Power Timing Parameters */
   0x62, 0x62,             /* Switcher Limit Voltages 98 V, 98 V */
   0x04,                   /* Charge pump disabled */
                           /* Charge Pump Overload Protection: Disabled */
   0x3C,                   /* Switcher Input 12 V */
   0x00, 0x00, 0x00, 0x00, /* Battery Backup Timing Parameters */
         0x00, 0x00,
   0x00, 0x00, 0x00,       /* Soft Startup Timing (Disabled) */
   0x00, 0x00              /* SwCtrl for Free Run and Battery Backup */

};

/************** DC Profile **************/

/* DC FXS Default - Use for all countries unless country file exists - 25mA Current Feed */
const VpProfileDataType DC_FXS_miSLIC_BB_DEF[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x01, 0x00,       /* DC Profile for Le9641 device */
   0x0C,                   /* Profile length = 12 + 4 = 16 bytes */
   0x02,                   /* Profile format version = 2 */
   0x03,                   /* MPI section length = 3 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC6, 0x92, 0x27,       /* DC Feed Parameters: ir_overhead = 100 ohm; */
                           /* VOC = 48 V; LI = 50 ohm; VAS = 8.78 V; ILA = 25 mA */
                           /* Maximum Voice Amplitude = 2.93 V */
/* Formatted Parameters Section -------------------------------------------- */
   0x1C,                   /* Switch Hook Threshold = 11 mA */
                           /* Ground-Key Threshold = 18 mA */
   0x84,                   /* Switch Hook Debounce = 8 ms */
                           /* Ground-Key Debounce = 16 ms */
   0x58,                   /* Low-Power Switch Hook Hysteresis = 2 V */
                           /* Ground-Key Hysteresis = 6 mA */
                           /* Switch Hook Hysteresis = 2 mA */
   0x80,                   /* Low-Power Switch Hook Threshold = 22 V */
   0x03,                   /* Floor Voltage = -20 V */
   0x00,                   /* R_OSP = 0 ohms */
   0x07                    /* R_ISP = 7 ohms */
};

/************** AC Profile **************/

/* AC FXS RF14 600R Normal Coefficients (Default)  A-D 0dB D-A -6dB */
const VpProfileDataType AC_FXS_RF14_600R_DEF[] =
{
  /* AC Profile */
 0xA4, 0x00, 0xF4, 0x4C, 0x01, 0x49, 0xCA, 0xF5, 0x98, 0xAA, 0x7B, 0xAB,
 0x2C, 0xA3, 0x25, 0xA5, 0x24, 0xB2, 0x3D, 0x9A, 0x2A, 0xAA, 0xA6, 0x9F,
 0x01, 0x8A, 0x1D, 0x01, 0xA3, 0xA0, 0x2E, 0xB2, 0xB2, 0xBA, 0xAC, 0xA2,
 0xA6, 0xCB, 0x3B, 0x45, 0x88, 0x2A, 0x20, 0x3C, 0xBC, 0x4E, 0xA6, 0x2B,
 0xA5, 0x2B, 0x3E, 0xBA, 0x8F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x42, 0xA1, 0xCB, 0x1B, 0xA3, 0xA8, 0xFB, 0x87, 0xAA,
 0xFB, 0x9F, 0xA9, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* 600ohm PTC=0 A-D=0dB D-A=-6dB */
const VpProfileDataType miSLIC_AC_600[] =
{
  /* AC Profile */
0x25, 0x00, 0x88, 0x4C, 0x01, 0x49, 0xCA, 0xF4, 0x98, 0x52, 0xDB, 0xA3,
0x2C, 0x37, 0xC5, 0xAA, 0x24, 0xFD, 0xBD, 0x9A, 0xAA, 0xAA, 0xB5, 0x9F,
0x01, 0x8A, 0x1D, 0x01, 0x44, 0xB0, 0x3C, 0xC2, 0x2B, 0xAA, 0xAB, 0xA2,
0x98, 0x4B, 0xCA, 0xA4, 0x88, 0xAA, 0x50, 0xB3, 0xAC, 0xB4, 0x37, 0x2A,
0xA5, 0x54, 0xAD, 0x2B, 0x87, 0x82, 0x2A, 0xA1, 0x80, 0xA2, 0x20, 0x50,
0x00, 0x86, 0x2A, 0x42, 0x22, 0x3B, 0x12, 0xA3, 0x2A, 0xE2, 0xAF, 0x5A,
0xE6, 0xAE, 0x3A, 0xE0, 0x96, 0xDD, 0x01, 0x00
};

/* 600ohm A-D=0dB D-A=-6dB for Gain ABS */
const VpProfileDataType AC_FXS_RF14_600_ABS[] =
{
  /* AC Profile */
0xFF, 0x00, 0xAD, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x24, 0x2A, 0xB7,
0x3B, 0x42, 0xB3, 0xC2, 0x23, 0xB2, 0xAB, 0x9A, 0xBA, 0xCB, 0xA3, 0x9F,
0x01, 0x8A, 0x1D, 0x01, 0x42, 0xB0, 0x01, 0x90, 0x01, 0x90, 0x01, 0x90,
0x01, 0x90, 0x01, 0x90, 0x88, 0xAA, 0x20, 0x33, 0xAC, 0xBA, 0x57, 0xE4,
0xB5, 0x33, 0xAD, 0xBA, 0x87, 0x82, 0x01, 0x11, 0x80, 0xA9, 0xF0, 0x50,
0x10, 0x86, 0x3A, 0x52, 0x23, 0x3B, 0x22, 0xA4, 0xC9, 0xF4, 0x9F, 0x3B,
0xF2, 0xAF, 0xDA, 0xF0, 0x96, 0x6D, 0x01, 0x00
};

/************** Ringing Profile **************/

/* Default Ringing 25Hz 50Vrms Tracking, AC Trip - Use for all countries unless country profile exists */
const VpProfileDataType RING_miSLIC_BB_DEF[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for Le9641 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x08, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x44, 0x3A, /* 24.9 Hz Sine; 1.41 CF; 70.70 Vpk; 0.00 V bias */
         0x9D, 0x00, 0x00, /* Ring trip cycles = 1; RTHALFCYC = 0 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0xAA,                   /* Ring Trip Method = AC; Threshold = 21.0 mA */
   0x04,                   /* Ringing Current Limit = 54 mA */
   0x4E,                   /* Fixed; Max Ringing Supply = 75 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* China Ringing 25Hz 60Vrms Fixed, AC Trip */
const VpProfileDataType miSLIC_BB_RING_25HZ_SINE[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for Le9661 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x08, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x44, 0x46, /* 24.9 Hz Sine; 1.41 CF; 84.80 Vpk; 0.00 V bias */
         0x4D, 0x00, 0x00, /* Ring trip cycles = 1; RTHALFCYC = 0 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0xA6,                   /* Ring Trip Method = AC; Threshold = 19.0 mA */
   0x00,                   /* Ringing Current Limit = 50 mA */
   0x50,                   /* Fixed; Max Ringing Supply = 85 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* China Ringing 25Hz 60Vpk Fixed, AC Trip */
const VpProfileDataType miSLIC_BB_RING_25HZ_Low[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for Le9661 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x08, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x44, 0x31, /* 24.9 Hz Sine; 1.41 CF; 60.00 Vpk; 0.00 V bias */
         0xBE, 0x00, 0x00, /* Ring trip cycles = 1; RTHALFCYC = 0 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0xA6,                   /* Ring Trip Method = AC; Threshold = 19.0 mA */
   0x00,                   /* Ringing Current Limit = 50 mA */
   0x4B,                   /* Fixed; Max Ringing Supply = 60 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Germany Ringing 25Hz 45Vrms Tracking, AC Trip */
const VpProfileDataType RING_miSLIC_BB_DE[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for Le9661 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x08, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x44, 0x34, /* 24.9 Hz Sine; 1.41 CF; 63.60 Vpk; 0.00 V bias */
         0xBA, 0x00, 0x00, /* Ring trip cycles = 1; RTHALFCYC = 0 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0xAA,                   /* Ring Trip Method = AC; Threshold = 21.0 mA */
   0x02,                   /* Ringing Current Limit = 54 mA */
   0x4C,                   /* Fixed; Max Ringing Supply = 65 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/************** Tone Profile **************/

/************** Cadence Profile **************/

/* China Ringing Signal Cadence */
const VpProfileDataType CR_CN[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x16, 0x01, 0x0D, 0x20, 0x14,
 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00, 0x20, 0x28,
 0x01, 0x05, 0x22, 0xF8, 0x44, 0x00
};

/* Germany Ringing Signal Cadence */
const VpProfileDataType CR_DE[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0xA0, 0x02, 0x01, 0x22, 0x80, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/************** Caller ID Profile **************/

/* DTMF CID Type1 */
const VpProfileDataType CID_TYPE1_CN_DTMF[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x14, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x00, /* Checksum Part of Message Data */
  0x00, 0x04, /* Length of Elements Data */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x0D  /* End of Transmission */
};


/* China Caller ID (Type 2 - Off-Hook) - Telcordia DTMF, without ACK */
const VpProfileDataType CID_TYPE2_CN_DTMF_NoACK[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x50, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x40, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* China Call Waiting Tone */
  /* 450.07 Hz, -7.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xCD, 0x27, 0xD5, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x90, /* Alert Tone Part 2 - Tone+Cadence = 400ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x06, 0x00, 0xA0, /* Silence Interval for 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};


/* China Caller ID (Type 2 - Off-Hook) - Telcordia DTMF */
const VpProfileDataType CID_TYPE2_CN_DTMF[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* China Call Waiting Tone */
  /* 450.07 Hz, -7.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xCD, 0x27, 0xD5, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x90, /* Alert Tone Part 2 - Tone+Cadence = 400ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* China Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_CN_FSK[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* China Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_CN_FSK[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* TypeII Caller ID - CN_SAS: 450Hz, -20dBm; +7dBm */
  /* 450.07 Hz, -13.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xCD, 0x13, 0xF7, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x90, /* Alert Tone Part 2 - Tone+Cadence = 400ms */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x04, /* Alert Tone */
  /* Type II Caller ID - CN_CAS: 2130Hz, -15dBm; 2750Hz, -15dBm: +7dBm */
  /* 2129.88 Hz, -8.00 dBm0, 2749.88 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x55, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x0A, 0x00, 0x46, /* Mark Signal for 70ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* China Caller ID Type II without ACK */
const VpProfileDataType CID_TYPE2_CN_FSK_NoACK[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x4C, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x3C, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* TypeII Caller ID - CN_SAS: 450Hz, -20dBm; +7dBm */
  /* 450.07 Hz, -13.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xCD, 0x13, 0xF7, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x90, /* Alert Tone Part 2 - Tone+Cadence = 400ms */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x04, /* Alert Tone */
  /* Type II Caller ID - CN_CAS: 2130Hz, -15dBm; 2750Hz, -15dBm: +7dBm */
  /* 2129.88 Hz, -8.00 dBm0, 2749.88 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x55, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x06, 0x00, 0xFA, /* Silence Interval for 250ms */
  0x00, 0x0A, 0x00, 0x46, /* Mark Signal for 70ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Germany Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_DE_FSK[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Germany Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_DE_FSK[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* TypeII Caller ID - CN_SAS: 450Hz, -20dBm; +7dBm */
  /* 450.07 Hz, -13.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xCD, 0x13, 0xF7, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x90, /* Alert Tone Part 2 - Tone+Cadence = 400ms */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x04, /* Alert Tone */
  /* Type II Caller ID - CN_CAS: 2130Hz, -15dBm; 2750Hz, -15dBm: +7dBm */
  /* 2129.88 Hz, -8.00 dBm0, 2749.88 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x55, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x0A, 0x00, 0x46, /* Mark Signal for 70ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Germany Caller ID Type II without ACK */
const VpProfileDataType CID_TYPE2_DE_FSK_NoACK[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x4C, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x3C, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* TypeII Caller ID - CN_SAS: 450Hz, -20dBm; +7dBm */
  /* 450.07 Hz, -13.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xCD, 0x13, 0xF7, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x90, /* Alert Tone Part 2 - Tone+Cadence = 400ms */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x04, /* Alert Tone */
  /* Type II Caller ID - CN_CAS: 2130Hz, -15dBm; 2750Hz, -15dBm: +7dBm */
  /* 2129.88 Hz, -8.00 dBm0, 2749.88 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x55, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x06, 0x00, 0xFA, /* Silence Interval for 250ms */
  0x00, 0x0A, 0x00, 0x46, /* Mark Signal for 70ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/************** Metering_Profile **************/

/* end of file profiles.c */

/************** Tone Profile **************/

const VpProfileDataType DTMF_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
};

const VpProfileDataType CN_Chinese_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone in China */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard dial tone 2 in China */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard busy tone in China */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard waiting tone in China */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard ringback tone in China */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* whistling tone */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
};

const VpProfileDataType DE_Germany_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone in Germany */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard dial tone 2 in Germany */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard busy tone in Germany */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard waiting tone in Germany */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard ringback tone in Germany */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* whistling tone */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
};

const VpProfileDataType ES_Spain_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone in Spain */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard dial tone 2 in Spain */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard busy tone in Spain */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard waiting tone in Spain */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard ringback tone in Spain */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* whistling tone */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
};

const VpProfileDataType UK_United_Kingdom_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone in UK */
   /* Sig gen A = 350.10 Hz at -10.00 dBm0 */
   /* Sig gen B = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x03, 0xBC, 0x1C,
            0x33, 0x04, 0xB1,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone 2 in UK */
   /* Sig gen A = 350.10 Hz at -10.00 dBm0 */
   /* Sig gen B = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x03, 0xBC, 0x1C,
            0x33, 0x04, 0xB1,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard busy tone in UK */
   /* Sig gen A = 399.90 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x44, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard waiting tone in UK */
   /* Sig gen A = 399.90 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x44, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard ringback tone in UK */
   /* Sig gen A = 399.90 Hz at -10.00 dBm0 */
   /* Sig gen B = 450.07 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x44, 0x1C,
            0x33, 0x04, 0xCD,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* whistling tone */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
};

const VpProfileDataType BR_Brazil_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone in Brazil */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard dial tone 2 in Brazil */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard busy tone in Brazil */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard waiting tone in Brazil */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard ringback tone in Brazil */
   /* Sig gen A = 425.17 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x89, 0x1C,
            0x33, 0x00, 0x00,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* whistling tone */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
};

const VpProfileDataType FR_France_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone in France */
   /* Sig gen A = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB1, 0x1C,
         0x33, 0x00, 0x00,
         0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard dial tone 2 in France */
   /* Sig gen A = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB1, 0x1C,
         0x33, 0x00, 0x00,
         0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard busy tone in France */
   /* Sig gen A = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB1, 0x1C,
         0x33, 0x00, 0x00,
         0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard waiting tone in France */
   /* Sig gen A = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB1, 0x1C,
         0x33, 0x00, 0x00,
         0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard ringback tone in France */
   /* Sig gen A = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB1, 0x1C,
         0x33, 0x00, 0x00,
         0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* whistling tone */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
};

const VpProfileDataType QA_Qatar_standard_tone[][20] = {
   {
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x03,                   /* Profile length = 3 + 4 = 7 bytes */
   0x02,                   /* Profile format version = 2 */
   0x00,                   /* MPI section length = 0 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   /* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* FM_EN = 0; WB = 0; Active Generators: */ 
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x07, 0x6F, 0x0E,
         0x22, 0x0C, 0xE5,
         0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1335.94 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0E, 0x40,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1208.86 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0C, 0xE5,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1476.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x0F, 0xC1,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Sig gen A = 696.90 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x07, 0x6F, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Sig gen A = 770.14 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x08, 0x37, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Sig gen A = 852.17 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x09, 0x17, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Sig gen A = 941.16 Hz at -16.00 dBm0 */
   /* Sig gen B = 1632.93 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x0A, 0x0A, 0x0E,
            0x22, 0x11, 0x6B,
            0x0E, 0x22,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone in Qutar */
   /* Sig gen A = 350.10 Hz at -10.00 dBm0 */
   /* Sig gen B = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x03, 0xBC, 0x1C,
            0x33, 0x04, 0xB1,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard dial tone 2 in Qutar */
   /* Sig gen A = 350.10 Hz at -10.00 dBm0 */
   /* Sig gen B = 439.82 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x03, 0xBC, 0x1C,
            0x33, 0x04, 0xB1,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard busy tone in Qutar */
   /* Sig gen A = 399.90 Hz at -10.00 dBm0 */
   /* Sig gen B = 450.07 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x44, 0x1C,
            0x33, 0x04, 0xCD,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* A standard waiting tone in Qutar */
   /* Sig gen A = 399.90 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x44, 0x1C,
            0x33, 0x00, 0x00,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
   {
   /* A standard ringing tone in Qutar */
   /* Sig gen A = 399.90 Hz at -10.00 dBm0 */
   /* Sig gen B = 450.07 Hz at -10.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0x44, 0x1C,
            0x33, 0x04, 0xCD,
            0x1C, 0x33,
   /* Formatted Parameters Section -------------------------------------------- */
      0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
   },
   {
   /* whistling tone */
   /* Sig gen A = 450.07 Hz at -16.00 dBm0 */
   /* Profile Header ---------------------------------------------------------- */
      0x0D, 0x02, 0x00,       /* Tone Profile for Le9661 device */
      0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
      0x02,                   /* Profile format version = 2 */
      0x0C,                   /* MPI section length = 12 bytes */
   /* Raw MPI Section --------------------------------------------------------- */
      0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
            0x04, 0xCD, 0x0E,
            0x22, 0x00, 0x00,
            0x13, 0xF7,
   /* Formatted Parameters Section -------------------------------------------- */
      0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
   },
};

/************** Cadence Profile **************/

const VpProfileDataType DTMF_standard_cadence[][20] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
};

const VpProfileDataType CN_Chinese_standard_cadence[][40] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* A standard dial tone in China */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard dial tone 2 in China */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard busy tone in China */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x46,
   0x00, 0x80, 0x20, 0x46, 0x40, 0x00
   },
   {
   /* A standard waiting tone in China */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x50,
   0x00, 0x80, 0x23, 0x20, 0x40, 0x00
   },
   {
   /* A standard ringback tone in China */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0xC8,
   0x00, 0x80, 0x23, 0x20, 0x40, 0x00
   },
   {
   /* whistling tone */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x14,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
};

const VpProfileDataType DE_Germany_standard_cadence[][40] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* A standard dial tone in Germany */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard dial tone 2 in Germany */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x81, 0x20, 0x50,
   0x00, 0x80, 0x20, 0x50, 0x00, 0x81, 0x20, 0x50, 0x00, 0x80, 0x20, 0x50,
   0x00, 0x81, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90, 0x40, 0x00
   },
   {
   /* A standard busy tone in Germany */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x60,
   0x00, 0x80, 0x20, 0x60, 0x40, 0x00
   },
   {
   /* A standard waiting tone in Germany */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x81, 0x20, 0x28,
   0x00, 0x80, 0x20, 0x28, 0x40, 0x01, 0x20, 0xA0, 0x00, 0x81, 0x20, 0x28,
   0x00, 0x80, 0x20, 0x28, 0x46, 0x01, 0x23, 0xC0, 0x46, 0x00
   },
   {
   /* A standard ringback tone in Germany */
     /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0xC8,
   0x00, 0x80, 0x23, 0x20, 0x40, 0x00
   },
   {
   /* whistling tone */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x14,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
};

const VpProfileDataType ES_Spain_standard_cadence[][40] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* A standard dial tone in Spain */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard dial tone 2 in Spain */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0xC8,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
   {
   /* A standard busy tone in Spain */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x28,
   0x00, 0x80, 0x20, 0x28, 0x40, 0x00
   },
   {
   /* A standard waiting tone in Spain */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x81, 0x20, 0x23,
   0x00, 0x80, 0x20, 0x23, 0x00, 0x81, 0x20, 0x23, 0x00, 0x80, 0x22, 0xBC,
   0x40, 0x00
   },
   {
   /* A standard ringback tone in Spain */
     /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x21, 0x2C,
   0x00, 0x80, 0x22, 0x58, 0x40, 0x00
   },
   {
   /* whistling tone */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x14,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
};

const VpProfileDataType UK_United_Kingdom_standard_cadence[][40] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* A standard dial tone in UK */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard dial tone 2 in UK */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard busy tone in UK */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x4B,
   0x00, 0x80, 0x20, 0x4B, 0x40, 0x00
   },
   {
   /* A standard waiting tone in UK */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x4B,
   0x00, 0x80, 0x20, 0x4B, 0x40, 0x00
   },
   {
   /* A standard ringback tone in UK */
     /* Cadence Profile */
   0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x83, 0x20, 0x50,
   0x00, 0x80, 0x20, 0x28, 0x00, 0x83, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
   0x40, 0x00
   },
   {
   /* whistling tone */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x14,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
};

const VpProfileDataType BR_Brazil_standard_cadence[][40] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* A standard dial tone in Brazil */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard dial tone 2 in Brazil */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0xC3,
   0x00, 0x80, 0x20, 0x0A, 0x40, 0x00
   },
   {
   /* A standard busy tone in Brazil */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x32,
   0x00, 0x80, 0x20, 0x32, 0x40, 0x00
   },
   {
   /* A standard waiting tone in Brazil */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x0A,
   0x00, 0x80, 0x20, 0xC8, 0x40, 0x00
   },
   {
   /* A standard ringback tone in Brazil */
     /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0xC8,
   0x00, 0x80, 0x23, 0x20, 0x40, 0x00
   },
   {
   /* whistling tone */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x14,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
};

const VpProfileDataType FR_France_standard_cadence[][40] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* A standard dial tone in France */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard dial tone 2 in France */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard busy tone in France */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x64,
   0x00, 0x80, 0x20, 0x64, 0x40, 0x00
   },
   {
   /* A standard waiting tone in France */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x81, 0x20, 0x3C,
   0x00, 0x80, 0x26, 0x40, 0x21, 0x90, 0x40, 0x00
   },
   {
   /* A standard ringback tone in France */
     /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x21, 0x2C,
   0x00, 0x80, 0x22, 0xBC, 0x40, 0x00
   },
   {
   /* whistling tone */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x14,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
};

const VpProfileDataType QA_Qatar_standard_cadence[][40] = {
   {
   /* QL_SLIC_NONE_TONE */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_1 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_2 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_3 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_4 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_5 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_6 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_7 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_8 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_9 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_0 */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_STAR */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_POUND */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_A */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_B */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_C */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* QL_SLIC_DTMF_D */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x83, 0x20, 0x00
   },
   {
   /* A standard dial tone in Qutar */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard dial tone 2 in Qutar */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
   },
   {
   /* A standard busy tone in Qutar */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0xA0,
   0x00, 0x80, 0x20, 0xA0, 0x40, 0x00
   },
   {
   /* A standard waiting tone in Qutar */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x81, 0x20, 0x3C,
   0x00, 0x80, 0x26, 0x40, 0x21, 0x90, 0x40, 0x00
   },
   {
   /* A standard ringing tone in Qutar */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x81, 0x20, 0x4C,
   0x00, 0x80, 0x20, 0x32, 0x40, 0x01, 0x21, 0x5E, 0x40, 0x00
   },
   {
   /* whistling tone */
   /* Cadence Profile */
   0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x14,
   0x00, 0x80, 0x20, 0x14, 0x40, 0x00
   },
};