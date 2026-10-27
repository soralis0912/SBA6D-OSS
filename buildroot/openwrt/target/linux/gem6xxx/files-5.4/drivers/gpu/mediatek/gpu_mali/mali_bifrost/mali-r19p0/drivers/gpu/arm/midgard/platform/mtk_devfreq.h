/*
 *
 * (C) COPYRIGHT 2012-2013, 2015-2016 ARM Limited. All rights reserved.
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






#ifndef _KBASE_GPU_DEVFREQ_H_
#define _KBASE_GPU_DEVFREQ_H_

int mtk_devfreq_target(struct device *dev,
        unsigned long *target_freq, u32 flags);
void mtk_gov_simple_ondemand_init(struct kbase_device *kbdev);
unsigned long model_static_power(struct devfreq *devfreq,
		unsigned long voltage_mv);
unsigned long model_dynamic_power(struct devfreq *devfreq,
		unsigned long freqHz,	unsigned long voltage_mv);
int modelget_real_power(struct devfreq *df, u32 *power,
			      unsigned long freqHz, unsigned long voltage_mv);
#endif				/* _KBASE_GPU_DEVFREQ_H_ */
