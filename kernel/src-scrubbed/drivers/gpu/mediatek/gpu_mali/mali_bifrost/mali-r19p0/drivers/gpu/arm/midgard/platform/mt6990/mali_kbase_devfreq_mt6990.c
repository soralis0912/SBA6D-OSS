/*
 *
 * (C) COPYRIGHT 2011-2016 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc.
 *
 */
#include <mali_kbase.h>
#include <linux/devfreq.h>
#include <linux/of.h>
#include <linux/clk.h>

#include "platform/mtk_devfreq.h"
#include "platform/mtk_governor.h"
#include "mtk_gpufreq.h"

#define DFSO_UPTHRESHOLD	(90)
#define DFSO_DOWNDIFFERENCTIAL	(5)

static int _devfreq_simple_ondemand_func(struct devfreq *df,
					unsigned long *freq)
{
	int err;
	struct devfreq_dev_status *stat;
	unsigned long long a, b;
	unsigned int dfso_upthreshold = DFSO_UPTHRESHOLD;
	unsigned int dfso_downdifferential = DFSO_DOWNDIFFERENCTIAL;
	struct devfreq_simple_ondemand_data *data = df->data;
	unsigned long max = (df->max_freq) ? df->max_freq : UINT_MAX;

	err = devfreq_update_stats(df);
	if (err)
		return err;

	stat = &df->last_status;

	if (data) {
		if (data->upthreshold)
			dfso_upthreshold = data->upthreshold;
		if (data->downdifferential)
			dfso_downdifferential = data->downdifferential;
	}
	if (dfso_upthreshold > 100 ||
	    dfso_upthreshold < dfso_downdifferential)
		return -EINVAL;

	/* Assume MAX if it is going to be divided by zero */
	if (stat->total_time == 0) {
		*freq = max;
		return 0;
	}

	/* Prevent overflow */
	if (stat->busy_time >= (1 << 24) || stat->total_time >= (1 << 24)) {
		stat->busy_time >>= 7;
		stat->total_time >>= 7;
	}

	/* Set MAX if it's busy enough */
	if (stat->busy_time * 100 >
	    stat->total_time * dfso_upthreshold) {
		*freq = max;
		return 0;
	}

	/* Set MAX if we do not know the initial frequency */
	if (stat->current_frequency == 0) {
		*freq = max;
		return 0;
	}

	/* Keep the current frequency */
	if (stat->busy_time * 100 >
	    stat->total_time * (dfso_upthreshold - dfso_downdifferential)) {
		*freq = stat->current_frequency;
		return 0;
	}

	/* Set the desired frequency based on the load */
	a = stat->busy_time;
	a *= stat->current_frequency;
	b = div_u64(a, stat->total_time);
	b *= 100;
	b = div_u64(b, (dfso_upthreshold - dfso_downdifferential / 2));
	*freq = (unsigned long) b;

	if (df->min_freq && *freq < df->min_freq)
		*freq = df->min_freq;
	if (df->max_freq && *freq > df->max_freq)
		*freq = df->max_freq;

	return 0;
}

static int devfreq_simple_ondemand_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	switch (event) {
        case DEVFREQ_GOV_START:
            devfreq_monitor_start(devfreq);
            break;
        case DEVFREQ_GOV_STOP:
            devfreq_monitor_stop(devfreq);
            break;
        case DEVFREQ_GOV_INTERVAL:
            devfreq_interval_update(devfreq, (unsigned int *)data);
            break;
        case DEVFREQ_GOV_SUSPEND:
            devfreq_monitor_suspend(devfreq);
            break;
        case DEVFREQ_GOV_RESUME:
            devfreq_monitor_resume(devfreq);
            break;
        default:
            break;
 	}
 	return 0;
 }
 
 static struct devfreq_governor devfreq_simple_ondemand = {
 	.name = "mtk_gpu_gov",
 	.get_target_freq = _devfreq_simple_ondemand_func,
 	.event_handler = devfreq_simple_ondemand_handler,
 };

void mtk_gov_simple_ondemand_init(struct kbase_device *kbdev)
{
	devfreq_add_governor(&devfreq_simple_ondemand);
}

int mtk_devfreq_target(struct device *dev, unsigned long *target_freq, u32 flags)
{
	struct kbase_device *kbdev = dev_get_drvdata(dev);
	struct dev_pm_opp *opp;
	unsigned long nominal_freq;
	unsigned int opp_index = 0;
	int ret;

	nominal_freq = *target_freq;
	/* In order to get the "nominal_freq" */
	opp = devfreq_recommended_opp(dev, &nominal_freq, flags);
	if (IS_ERR_OR_NULL(opp)) {
		dev_err(dev, "Failed to get opp (%ld)\n", PTR_ERR(opp));
		return PTR_ERR(opp);
	}
	dev_pm_opp_put(opp);

	/* Translate to opp index */
	switch(nominal_freq) {
		case 780000000:
			opp_index = 0;
			break;
		case 570000000:
			opp_index = 1;
			break;
		case 360000000:
			opp_index = 2;
			break;
		case 300000000:
			opp_index = 3;
			break;
		default:
			opp_index = 0;
	}

	/* Adjust freq */
	ret = mt_gpufreq_target(opp_index);

	/* 
     * Write back the processed freq in devfreq:target_freq
     * and mali:current_nominal_freq                        
     */
	kbdev->current_nominal_freq = nominal_freq;
	*target_freq = nominal_freq;

	return 0;
}

unsigned long model_static_power(struct devfreq *devfreq,
		unsigned long voltage_mv)
{
	(void)(voltage_mv);
	return mt_gpufreq_get_leakage_mw();
}

unsigned long model_dynamic_power(struct devfreq *devfreq,
		unsigned long freqHz,	unsigned long voltage_mv)
{
	return mt_gpufreq_get_dyn_power(freqHz/1000, voltage_mv * 100);
}
/* look-up power table with current freq */
int modelget_real_power(struct devfreq *df, u32 *power,
			      unsigned long freqHz, unsigned long voltage_mv)

{
	int opp_idx;

	(void)(voltage_mv);
	(void)(df);

	opp_idx = mt_gpufreq_get_opp_idx_by_freq(freqHz);
	if (power) /* return power */
		*power = mt_gpufreq_get_pow_by_idx(opp_idx);
	/* assume success */
	return 0;
}