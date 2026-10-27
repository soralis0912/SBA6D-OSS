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
#include "si321xx.h"
#include "inc/proslic.h"
#include "timer.h"
#include "spi.h"

#include "../../mediatek/mt6990/mt6990-afe-common.h"

static ctrl_S spiGciObj; /* User¡¦s control interface object */
static systemTimer_S timerObj; /* User¡¦s timer object */
/* Define ProSLIC control interface object */
static controlInterfaceType *ProHWIntf;
/* Define array of ProSLIC device objects */
static ProslicDeviceType *ProSLICDevices[NUMBER_OF_PROSLIC];
/* Define array of ProSLIC channel object pointers */
proslicChanType_ptr arrayOfProslicChans[NUMBER_OF_CHAN];

static void ProSLIC_LP(bool enable)
{
	int32 ret = 0;

	printk("%s++ enable = %d\n", __func__,enable);
	ret = ProSLIC_ReadReg(arrayOfProslicChans[0], 47);

	if (enable) {
		ret = ret | 0x4;
	} else {
		ret = ret & 0xFFFFFFFB;
	}

	ProSLIC_WriteReg(arrayOfProslicChans[0], 47, ret);

	// For debug
	ProSLIC_ReadReg(arrayOfProslicChans[0], 47);
}

int si321xx_ProSLIC_HWInit(void)
{
	int32 i, result= 0;
	int error = 0;
	//int32 i=0;

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
	ProSLIC_createControlInterface(&ProHWIntf);
	/*
	** Step 3: (required)
	** Create ProSLIC Device Objects
	*/
	for(i=0;i<NUMBER_OF_PROSLIC;i++)
	{
		ProSLIC_createDevice(&(ProSLICDevices[i]));
	}
	/*
	** Step 4: (required)
	** Create and initialize ProSLIC channel objects
	** Also initialize array pointers to user¡¦s proslic channel object
	** members to simplify initialization process.
	*/
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_createChannel(&(arrayOfProslicChans[i]));
		ProSLIC_SWInitChan(arrayOfProslicChans[i],i,PROSLIC_DEVICE_TYPE,
		ProSLICDevices[i/CHAN_PER_DEVICE],ProHWIntf);
		ProSLIC_setSWDebugMode(arrayOfProslicChans[i],TRUE); /* optional */
	}
	/*
	** Step 5: (required)
	** Establish linkage between host objects/functions and
	** ProSLIC API
	*/
	ProSLIC_setControlInterfaceCtrlObj (ProHWIntf, &spiGciObj);
	ProSLIC_setControlInterfaceReset (ProHWIntf, ctrl_ResetWrapper);
	ProSLIC_setControlInterfaceWriteRegister (ProHWIntf, ctrl_WriteRegisterWrapper);
	ProSLIC_setControlInterfaceReadRegister (ProHWIntf, ctrl_ReadRegisterWrapper);
	ProSLIC_setControlInterfaceWriteRAM (ProHWIntf, ctrl_WriteRAMWrapper);
	ProSLIC_setControlInterfaceReadRAM (ProHWIntf, ctrl_ReadRAMWrapper);
	ProSLIC_setControlInterfaceTimerObj (ProHWIntf, &timerObj);
	ProSLIC_setControlInterfaceDelay (ProHWIntf, time_DelayWrapper);
	ProSLIC_setControlInterfaceTimeElapsed (ProHWIntf, time_TimeElapsedWrapper);
	ProSLIC_setControlInterfaceGetTime (ProHWIntf, time_GetTimeWrapper);
	ProSLIC_setControlInterfaceSemaphore (ProHWIntf, NULL);
	/*
	** Step 6: (system dependent)
	** Assert hardware Reset ¡V ensure VDD, PCLK, and FSYNC are present and stable
	** before releasing reset
	*/
	ProSLIC_Reset(arrayOfProslicChans[0]);

	//initialize SPI interface
	//if (SPI_Init (NULL) ==  FALSE){
	//	printk ("%s : Cannot connect\n",__FUNCTION__);
		//return 0;
	//}

	/*
	** Step 7: (required)
	** Initialize device (loading of general parameters, calibrations,
	** dc-dc powerup, etc.)
	*/
	error = ProSLIC_Init(arrayOfProslicChans,NUMBER_OF_CHAN);
	if(error) {
		printk("ProSLIC_Init error\n");
		return error;
	}
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		if(arrayOfProslicChans[i]->error!=0)
		{
			printk("ProSLIC_Init[%d] ERR=%d\n",i,arrayOfProslicChans[i]->error);
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
	error = ProSLIC_LBCal(arrayOfProslicChans,NUMBER_OF_CHAN);
	if(error) {
		printk("ProSLIC_LBCal error\n");
		return error;
	}
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_GetLBCalResultPacked(arrayOfProslicChans[i], &result);
		printk("LBCal=0x%08X\n",result);
		if(!result)
			return -1;
	}

	if(QUECTEL_ETDM_RATE == 16000)
    {
		/*Add step for Wideband*/
		for(i=0;i<NUMBER_OF_CHAN;i++)
		{
			printk("ProSLIC SI321XX set option for Wideband, 16K\n");
			ProSLIC_ZsynthSetup(arrayOfProslicChans[i],WB_ZSYN_600_0_0_20_0);    //Wide band
			ProSLIC_AudioGainSetup(arrayOfProslicChans[i],-2,-4,WB_ZSYN_600_0_0_20_0);
		}
	}
	else if(QUECTEL_ETDM_RATE == 8000)
	{
		for(i=0;i<NUMBER_OF_CHAN;i++)
		{
			printk("ProSLIC SI321XX set option for Narrowband, 8K\n");
			ProSLIC_ZsynthSetup(arrayOfProslicChans[i],ZSYN_600_0_0_30_0);		//Narrow band
			ProSLIC_AudioGainSetup(arrayOfProslicChans[i],-2,-4,ZSYN_600_0_0_30_0);
		}
	}

	/*
	** Step 9: (design dependent)
	** Load custom configuration presets(generated using
	** ProSLIC API Config Tool)
	*/

	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_PCMTimeSlotSetup(arrayOfProslicChans[i],0,0);
		ProSLIC_DCFeedSetup(arrayOfProslicChans[i],DCFEED_48V_20MA);
#ifdef SI3218x
		ProSLIC_RingSetup(arrayOfProslicChans[i],RING_F20_35VRMS_0VDC_LPR);
#else
		ProSLIC_RingSetup(arrayOfProslicChans[i],RING_F20_45VRMS_0VDC_LPR);
#endif
		if(QUECTEL_ETDM_RATE == 16000)
    	{
			ProSLIC_PCMSetup(arrayOfProslicChans[i],PCM_16LIN_WB);
		}
		else if(QUECTEL_ETDM_RATE == 8000)
		{
			ProSLIC_PCMSetup(arrayOfProslicChans[i],PCM_16LIN);/*PCM_DEFAULT_CONFIG*/    //Narrow band
		}
	}

	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		ProSLIC_PCMStart(arrayOfProslicChans[i]);
	}
	for(i=0;i<NUMBER_OF_CHAN;i++)
	{
		error = ProSLIC_SetLinefeedStatus(arrayOfProslicChans[i],LF_FWD_ACTIVE);
		if(error) {
			printk("Set LF_FWD_ACTIVE error\n");
			return error;
		}
	}
	//mdelay(500);
	//ProSLIC_LP(true);
	//mdelay(500);
	return error;
}

static int si321xx_component_probe(struct snd_soc_component *component)
{
	dev_info(component->dev, "%s\n", __func__);
	return 0;
}

static void si321xx_component_remove(struct snd_soc_component *component)
{
	struct si321xx_chip *chip = snd_soc_component_get_drvdata(component);

	dev_info(component->dev, "%s\n", __func__);

	chip->component = NULL;
	ProSLIC_LP(false);
}
static int si321xx_component_get_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = -EINVAL;

	//printk(KERN_INFO "PROSLIC register si321xx_component_get_volsw\n");

	if (!strcmp(kcontrol->id.name, "Chip_Rev")) {
		ucontrol->value.integer.value[0] = 0x0f;
		ret = 0;
	}
	return ret;
}

static const struct snd_soc_dapm_widget si321xx_component_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("VINP"),
	SND_SOC_DAPM_OUTPUT("VOUTP"),
};

static const struct snd_soc_dapm_route si321xx_component_dapm_routes[] = {
	{ "VOUTP", NULL, "aif_playback"},
	{ "aif_capture", NULL, "VINP"},
};

static const char * const si321xx_lpbk_str[] = {
	"off", "on"
};

static const struct soc_enum si321xx_lpbk_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(si321xx_lpbk_str),
			    si321xx_lpbk_str),
};

static int si321xx_lpbk_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{/*
	ucontrol->value.integer.value[0] =
		ProSLIC_ReadReg(arrayOfProslicChans[0],47);
*/
	return 0;
}

static int si321xx_lpbk_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{/*
	unsigned int ret;

	if (ucontrol->value.integer.value[0]) {
		//ProSLIC_PrintDebugData(arrayOfProslicChans[0]);
		ProSLIC_LP(true);
	} else
		ProSLIC_LP(false);
	ret = ProSLIC_ReadReg(arrayOfProslicChans[0], 47);
	printk(KERN_INFO "PROSLIC register 0x43 = 0x%x\n",ret);
*/
	return 0;
}

/* proslic regcontrol */
static int si321xx_reg_get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
/*
	ucontrol->value.integer.value[0] =
		ProSLIC_ReadReg(arrayOfProslicChans[0],47);
*/
	return 0;
}

static int si321xx_reg_set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{/*
	int reg, data, ret = 0;

	reg = ucontrol->value.integer.value[0];
	data = ucontrol->value.integer.value[1];

	ret = ProSLIC_ReadReg(arrayOfProslicChans[0], (uInt8)reg);

	printk(KERN_INFO "PROSLIC bf register %d = (%d) %d \n",reg,data, ret);
	ProSLIC_WriteReg(arrayOfProslicChans[0], reg, data);
	ret = ProSLIC_ReadReg(arrayOfProslicChans[0], (uInt8)reg);
*/
	return 0;
}

static const struct snd_kcontrol_new si321xx_component_snd_controls[] = {
	SOC_SINGLE_EXT("Chip_Rev", SND_SOC_NOPM, 0, 16, 0,
					si321xx_component_get_volsw, NULL),
	SOC_DOUBLE_EXT("proslic_reg", SND_SOC_NOPM, 0, 1, 0xffff, 0,
		       		si321xx_reg_get, si321xx_reg_set),
	SOC_ENUM_EXT("proslic_lpbk", si321xx_lpbk_enum[0],
				 si321xx_lpbk_get, si321xx_lpbk_set),
};

static const struct snd_soc_component_driver si321xx_component_driver = {
	.probe = si321xx_component_probe,
	.remove = si321xx_component_remove,

	.controls = si321xx_component_snd_controls,
	.num_controls = ARRAY_SIZE(si321xx_component_snd_controls),
	.dapm_widgets = si321xx_component_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(si321xx_component_dapm_widgets),
	.dapm_routes = si321xx_component_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(si321xx_component_dapm_routes),

	.idle_bias_on = false,
};

static int si321xx_component_aif_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "%s\n", __func__);
	return 0;
}

static int si321xx_component_aif_hw_params(struct snd_pcm_substream *substream,
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

static const struct snd_soc_dai_ops si321xx_component_aif_ops = {
	.startup = si321xx_component_aif_startup,
	.hw_params = si321xx_component_aif_hw_params,
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

static struct snd_soc_dai_driver si321xx_codec_dai = {
	.name = "proslic_spi-aif",
	.playback = {
		.stream_name	= "aif_playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= STUB_RATES,
		.formats	= STUB_FORMATS,
	},
	.capture = {
		.stream_name	= "aif_capture",
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
	.ops = &si321xx_component_aif_ops,
};

int si321xx_spi_probe(struct spi_device *spi, struct spi_driver *spi_drv)
{
	int ret;

	printk(KERN_INFO "PROSLIC si321xx_spi_probe\n");

	ret = proslic_spi_probe(spi,spi_drv);

	return snd_soc_register_component(&spi->dev, &si321xx_component_driver,
				      &si321xx_codec_dai, 1);
}
EXPORT_SYMBOL(si321xx_spi_probe);

int si321xx_spi_remove(struct spi_device *spi)
{
	proslic_spi_remove(spi);
	snd_soc_unregister_component(&spi->dev);

	return 0;
}
EXPORT_SYMBOL(si321xx_spi_remove);

int si321xx_init_thread(void *arg)
{
	si321xx_ProSLIC_HWInit();
	do_exit(0);
	return 0;
}
EXPORT_SYMBOL(si321xx_init_thread);

MODULE_DESCRIPTION("ASoC si321xx CODEC SPI driver");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_LICENSE("GPL");

