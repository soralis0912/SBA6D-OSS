/*
 * ak4940.h  --  audio driver for ak4940
 *
 * Copyright (C) 2013 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      18/08/XX	    1.0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef _AK4940_H
#define _AK4940_H

#define AK4940_CLOCK_PLAYBACK	1
#define AK4940_CLOCK_CAPTURE	2
#define AK4940_CLOCK_OTHER		4

#define AK4940_00_CLOCK_MANAGEMENT		0x00
#define AK4940_01_AUDIO_IF_SETTING      0x01
#define AK4940_02_ADC_CONTROL			0x02
#define AK4940_03_LINEOUT_CONTROL		0x03
#define AK4940_04_POWER_MANAGEMENT		0x04
#define AK4940_05_INPUT_GAIN			0x05
#define AK4940_06_ADC_DIGITAL_VOLUME	0x06
#define AK4940_07_DAC_DIGITAL_VOLUME	0x07
#define AK4940_08_SOFTMUTE_CONTROL		0x08
#define AK4940_MAX_REGISTER			AK4940_08_SOFTMUTE_CONTROL

/* AK4940_01_AUDIO_IF_SETTING (0x01) Fields */
#define AK4940_DIF_64FS				0x00
#define AK4940_DIF_48FS				0x01
#define AK4940_DIF_32FS				0x02

#define AK4940_DIF_24MSB_MODE		0x00
#define AK4940_DIF_24LSB_MODE		0x14
#define AK4940_DIF_20LSB_MODE		0x28
#define AK4940_DIF_16LSB_MODE		0x3C
#define AK4940_DIF_24I2S_MODE	 	0x40
#define AK4940_DIF_16I2S_MODE	 	0x4E

#define AK4940_DIF_24PCMSHORT_MODE	0x80
#define AK4940_DIF_24PCMLONG_MODE	0xC0

#define AK4940_MASK_LRIF_DOIF		0xFC
#define AK4940_MASK_BITFS			0x03

#endif
