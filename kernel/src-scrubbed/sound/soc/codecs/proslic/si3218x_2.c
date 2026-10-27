// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/init.h>


#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <linux/regmap.h>
#include <linux/spi/spi.h>


#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#include "proslic_sys.h"
#include "proslic_sys2.h"
#include "si321xx.h"
#include "si32185_2.h"
#include "inc/proslic.h"
#include "timer.h"

#include "spi.h"
#include "spi2.h"
#include "../../mediatek/mt6990/mt6990-afe-common.h"

#ifdef SI3218X
#include "inc/si3218x.h"
#include "inc/si3218x_intf.h"
#endif


ctrl_S spiGciObj2; /* User¡¦s control interface object */
systemTimer_S timerObj2; /* User¡¦s timer object */
/* Define ProSLIC control interface object */
controlInterfaceType *ProHWIntf2;
/* Define array of ProSLIC device objects */
ProslicDeviceType *ProSLICDevices2[NUMBER_OF_PROSLIC];
/* Define array of ProSLIC channel object pointers */
proslicChanType_ptr arrayOfProslicChans2[NUMBER_OF_CHAN];

void ProSLIC_LP2(bool enable)
{
	int32 ret = 0;

	printk("%s++ enable = %d\n", __func__,enable);
	ret = ProSLIC_ReadReg(arrayOfProslicChans2[0], 47);

	if (enable) {
		ret = ret | 0x4;
	} else {
		ret = ret & 0xFFFFFFFB;
	}

	ProSLIC_WriteReg(arrayOfProslicChans2[0], 47, ret);

	// For debug
	ProSLIC_ReadReg(arrayOfProslicChans2[0], 47);
}
EXPORT_SYMBOL(ProSLIC_LP2);

int ProSLIC_HWInit2(void)
{
	int32 i, result= 0;
	int error = 0;

	printk ("%s()\n",__FUNCTION__);
	/*
	** Step 1: (optional)
	** Initialize user¡¦s control interface and timer objects ¡V this
	** may already be done, if not, do it here
	*/
	/*
	** Step 2: (required)
	** Create ProSLIC Control Interface Object
	*/
	ProSLIC_createControlInterface(&ProHWIntf2);
	/*
	** Step 3: (required)
	** Create ProSLIC Device Objects
	*/
	for(i=0;i<NUMBER_OF_PROSLIC;i++)
	{
		ProSLIC_createDevice(&(ProSLICDevices2[i]));
	}
	/*
	** Step 4: (required)
	** Create and initialize ProSLIC channel objects
	** Also initialize array pointers to user¡¦s proslic channel object
	** members to simplify initialization process.
	*/
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_createChannel(&(arrayOfProslicChans2[i]));
		ProSLIC_SWInitChan(arrayOfProslicChans2[i],i,PROSLIC_DEVICE_TYPE,
		ProSLICDevices2[i/CHAN_PER_DEVICE],ProHWIntf2);
		ProSLIC_setSWDebugMode(arrayOfProslicChans2[i],TRUE); /* optional */
	}
	/*
	** Step 5: (required)
	** Establish linkage between host objects/functions and
	** ProSLIC API
	*/
	ProSLIC_setControlInterfaceCtrlObj (ProHWIntf2, &spiGciObj2);
	ProSLIC_setControlInterfaceReset (ProHWIntf2, ctrl_ResetWrapper2);
	ProSLIC_setControlInterfaceWriteRegister (ProHWIntf2, ctrl_WriteRegisterWrapper2);
	ProSLIC_setControlInterfaceReadRegister (ProHWIntf2, ctrl_ReadRegisterWrapper2);
	ProSLIC_setControlInterfaceWriteRAM (ProHWIntf2, ctrl_WriteRAMWrapper2);
	ProSLIC_setControlInterfaceReadRAM (ProHWIntf2, ctrl_ReadRAMWrapper2);
	ProSLIC_setControlInterfaceTimerObj (ProHWIntf2, &timerObj2);
	ProSLIC_setControlInterfaceDelay (ProHWIntf2, time_DelayWrapper);
	ProSLIC_setControlInterfaceTimeElapsed (ProHWIntf2, time_TimeElapsedWrapper);
	ProSLIC_setControlInterfaceGetTime (ProHWIntf2, time_GetTimeWrapper);
	ProSLIC_setControlInterfaceSemaphore (ProHWIntf2, NULL);
	/*
	** Step 6: (system dependent)
	** Assert hardware Reset ¡V ensure VDD, PCLK, and FSYNC are present and stable
	** before releasing reset
	*/
	/* To decrease boot up time without SLIC */     //830 add
	// if (ProSLIC_identifyChannelType(arrayOfProslicChans2[0]) != PROSLIC)
	// 	return RC_CHANNEL_TYPE_ERR;

	ProSLIC_Reset(arrayOfProslicChans2[0]);

	//initialize SPI interface
	//if (SPI_Init2 (NULL) ==  FALSE){
	//	printk ("%s : Cannot connect\n",__FUNCTION__);
		//return 0;
	//}

	/*
	** Step 7: (required)
	** Initialize device (loading of general parameters, calibrations,
	** dc-dc powerup, etc.)
	*/
	error = ProSLIC_Init(arrayOfProslicChans2,NUMBER_OF_CHAN);
	if(error) {
		printk("ProSLIC_Init error\n");
		return error;
	}
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		if(arrayOfProslicChans2[i]->error!=0)
		{
			printk("ProSLIC_Init[%d] ERR=%d\n",i,arrayOfProslicChans2[i]->error);
			return 0;
		}
	}


	/*
	** Step 8: (design dependent)
	** Execute longitudinal balance calibration
	** or reload coefficients from factory LB cal
	**
	** Note: all batteries should be up and stable prior to
	** executing the lb cal
	*/
	error = ProSLIC_LBCal(arrayOfProslicChans2,NUMBER_OF_CHAN);
	if(error) {
		printk("ProSLIC_LBCal error\n");
		return error;
	}
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_GetLBCalResultPacked(arrayOfProslicChans2[i], &result);
		printk("LBCal=0x%08X\n",result);
	}

	if(QUECTEL_ETDM_RATE == 16000)
    {
		/*Add step for Wideband*/
		for(i=0;i<NUMBER_OF_CHAN;i++)
		{
			printk("ProSLIC SI3218x set option for Wideband, 16K\n");
			ProSLIC_ZsynthSetup(arrayOfProslicChans2[i],WB_ZSYN_600_0_0_20_0);    //Wide band
			ProSLIC_AudioGainSetup(arrayOfProslicChans2[i],-2,-4,WB_ZSYN_600_0_0_20_0);
		}
	}
	else if(QUECTEL_ETDM_RATE == 8000)
	{
		for(i=0;i<NUMBER_OF_CHAN;i++)
		{
			printk("ProSLIC SI3218x set option for Narrowband, 8K\n");
			ProSLIC_ZsynthSetup(arrayOfProslicChans2[i],ZSYN_600_0_0_30_0);		//Narrow band
			ProSLIC_AudioGainSetup(arrayOfProslicChans2[i],-2,-4,ZSYN_600_0_0_30_0);
		}
	}

	/*
	** Step 9: (design dependent)
	** Load custom configuration presets(generated using
	** ProSLIC API Config Tool)
	*/

	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_PCMTimeSlotSetup(arrayOfProslicChans2[i],0,0);
		ProSLIC_DCFeedSetup(arrayOfProslicChans2[i],DCFEED_48V_20MA);
		ProSLIC_RingSetup(arrayOfProslicChans2[i],RING_F20_35VRMS_0VDC_LPR);
		if(QUECTEL_ETDM_RATE == 16000)
    	{
			ProSLIC_PCMSetup(arrayOfProslicChans2[i],PCM_16LIN_WB);
		}
		else if(QUECTEL_ETDM_RATE == 8000)
		{
			ProSLIC_PCMSetup(arrayOfProslicChans2[i],PCM_16LIN);/*PCM_DEFAULT_CONFIG*/    //Narrow band
		}
		ProSLIC_ToneGenSetup(arrayOfProslicChans2[i],TONEGEN_FCC_DIAL);
	}

	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_PCMStart(arrayOfProslicChans2[i]);
	}
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		error = ProSLIC_SetLinefeedStatus(arrayOfProslicChans2[i],LF_FWD_ACTIVE);
		if(error) {
			printk("Set LF_FWD_ACTIVE error\n");
			return error;
		}
	}
	//mdelay(500);
	//ProSLIC_LP2(true);
	//mdelay(500);
	return error;
}
EXPORT_SYMBOL(ProSLIC_HWInit2);

static int si3218x_component_probe(struct snd_soc_component *component)
{
	dev_info(component->dev, "%s\n", __func__);
	return 0;
}

static void si3218x_component_remove(struct snd_soc_component *component)
{
	struct si321xx_chip *chip = snd_soc_component_get_drvdata(component);

	dev_info(component->dev, "%s\n", __func__);

	chip->component = NULL;
	ProSLIC_LP2(false);
}
static int si3218x_component_get_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = -EINVAL;

	//printk(KERN_INFO "PROSLIC register si3218x_component_get_volsw\n");

	if (!strcmp(kcontrol->id.name, "Chip_Rev2")) {
		ucontrol->value.integer.value[0] = 0x0f;
		ret = 0;
	}
	return ret;
}

static const struct snd_soc_dapm_widget si3218x_component_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("VINP2"),
	SND_SOC_DAPM_OUTPUT("VOUTP2"),
};

static const struct snd_soc_dapm_route si3218x_component_dapm_routes[] = {
	{ "VOUTP2", NULL, "aif_playback2"},
	{ "aif_capture2", NULL, "VINP2"},
};

static const char * const si3218x_lpbk_str[] = {
	"off", "on"
};

static const struct soc_enum si3218x_lpbk_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(si3218x_lpbk_str),
			    si3218x_lpbk_str),
};

static int si3218x_lpbk_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
#if 0
	ucontrol->value.integer.value[0] =
		ProSLIC_ReadReg(arrayOfProslicChans2[0],47);
#endif
	return 0;
}

static int si3218x_lpbk_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
#if 0
	unsigned int ret;

	if (ucontrol->value.integer.value[0]) {
		//ProSLIC_PrintDebugData(arrayOfProslicChans2[0]);
		ProSLIC_LP2(true);
	} else
		ProSLIC_LP2(false);
	ret = ProSLIC_ReadReg(arrayOfProslicChans2[0], 47);
	printk(KERN_INFO "PROSLIC register 0x43 = 0x%x\n",ret);
#endif
	return 0;
}

/* proslic regcontrol */
static int si3218x_reg_get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
#if 0
	ucontrol->value.integer.value[0] =
		ProSLIC_ReadReg(arrayOfProslicChans2[0],47);
    //ProSLIC_PrintDebugData(arrayOfProslicChans2[0]);
    //ProSLIC_PrintDebugReg(arrayOfProslicChans2[0]);
#endif
	return 0;
}

static int si3218x_reg_set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
#if 0
	int reg, data, ret = 0;

	reg = ucontrol->value.integer.value[0];
	data = ucontrol->value.integer.value[1];

	ret = ProSLIC_ReadReg(arrayOfProslicChans2[0], (uInt8)reg);

	printk(KERN_INFO "PROSLIC bf register %d = (%d) %d \n",reg,data, ret);
	ProSLIC_WriteReg(arrayOfProslicChans2[0], reg, data);
	ret = ProSLIC_ReadReg(arrayOfProslicChans2[0], (uInt8)reg);
#endif
	return 0;
}

#if 1
static const struct snd_kcontrol_new si3218x_component_snd_controls[] = {
	SOC_SINGLE_EXT("Chip_Rev2", SND_SOC_NOPM, 0, 16, 0,
					si3218x_component_get_volsw, NULL),
	SOC_DOUBLE_EXT("proslic_reg2", SND_SOC_NOPM, 0, 1, 0xffff, 0,
		       		si3218x_reg_get, si3218x_reg_set),
	SOC_ENUM_EXT("proslic_lpbk2", si3218x_lpbk_enum[0],
				 si3218x_lpbk_get, si3218x_lpbk_set),
};
#endif

static const struct snd_soc_component_driver si3218x_component_driver = {
	.probe = si3218x_component_probe,
	.remove = si3218x_component_remove,

#if 1
	.controls = si3218x_component_snd_controls,
	.num_controls = ARRAY_SIZE(si3218x_component_snd_controls),
#endif
	.dapm_widgets = si3218x_component_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(si3218x_component_dapm_widgets),
	.dapm_routes = si3218x_component_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(si3218x_component_dapm_routes),

	.idle_bias_on = false,
};

static int si3218x_component_aif_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "%s\n", __func__);
	return 0;
}

static int si3218x_component_aif_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
	int word_len = params_physical_width(hw_params);
	int aud_bit = params_width(hw_params);

	dev_dbg(dai->dev, "%s: ++\n", __func__);
	dev_dbg(dai->dev, "format: 0x%08x\n", params_format(hw_params));
	dev_dbg(dai->dev, "rate: 0x%08x\n", params_rate(hw_params));
	dev_dbg(dai->dev, "word_len: %d, aud_bit: %d\n", word_len, aud_bit);
	if (word_len > 32 || word_len < 16) {
		dev_err(dai->dev, "not supported word length\n");
		return -ENOTSUPP;
	}

	dev_dbg(dai->dev, "%s: --\n", __func__);
	return 0;
}

static const struct snd_soc_dai_ops si3218x_component_aif_ops = {
	.startup = si3218x_component_aif_startup,
	.hw_params = si3218x_component_aif_hw_params,
};

#if (QUECTEL_ETDM_RATE == 16000)
#define STUB_RATES	SNDRV_PCM_RATE_16000  //Wide band
#else
#define STUB_RATES	SNDRV_PCM_RATE_8000_192000    //Narrow band
#endif
#define STUB_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_U16_LE | \
			SNDRV_PCM_FMTBIT_S24_LE | \
			SNDRV_PCM_FMTBIT_U24_LE | \
			SNDRV_PCM_FMTBIT_S32_LE | \
			SNDRV_PCM_FMTBIT_U32_LE)

static struct snd_soc_dai_driver si3218x_codec_dai = {
	.name = "proslic_spi-aif2",
	.playback = {
		.stream_name	= "aif_playback2",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= STUB_RATES,
		.formats	= STUB_FORMATS,
	},
	.capture = {
		.stream_name	= "aif_capture2",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates = STUB_RATES,
		.formats = STUB_FORMATS,
	},
	/* dai properties */
	.symmetric_rates = 1,
	.symmetric_channels = 1,
	.symmetric_samplebits = 1,
	/* dai operations */
	.ops = &si3218x_component_aif_ops,
};

int si3218x2_spi_probe(struct spi_device *spi, struct spi_driver *spi_drv)
{
	int ret;

	printk(KERN_INFO "PROSLIC si3218x_spi_probe\n");

	ret = proslic_spi_probe2(spi,spi_drv);
	/* ret will always < 0 here because of PCLK off */

	return snd_soc_register_component(&spi->dev, &si3218x_component_driver,
				      &si3218x_codec_dai, 1);
}
EXPORT_SYMBOL(si3218x2_spi_probe);

int si3218x2_spi_remove(struct spi_device *spi)
{
	proslic_spi_remove2(spi);
	snd_soc_unregister_component(&spi->dev);

	return 0;
}
EXPORT_SYMBOL(si3218x2_spi_remove);

int si3218x2_init_thread(void *arg)
{
	ProSLIC_HWInit2();
	do_exit(0);
	return 0;
}
EXPORT_SYMBOL(si3218x2_init_thread);

MODULE_DESCRIPTION("ASoC si3218x CODEC SPI driver");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_LICENSE("GPL");

