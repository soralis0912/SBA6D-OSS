/*
 *
 * (C) COPYRIGHT 2011-2017 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_cpu_mt6890.h"
#include "mali_kbase_config_platform.h"
#include "platform/mtk_platform_common.h"
#include "ged_dvfs.h"
#include "mtk_gpufreq.h"
#include "mboot_params.h"

#ifdef ENABLE_MTK_DEBUG
#include <mtk_gpu_log.h>
#include "mtk_idle.h"
#endif

#define MALI_TAG						"[GPU/MALI]"
#define mali_pr_info(fmt, args...)		pr_info(MALI_TAG"[INFO]"fmt, ##args)
#define mali_pr_debug(fmt, args...)		pr_debug(MALI_TAG"[DEBUG]"fmt, ##args)
#define MT_GPUFREQ_SRAM_DEBUG

DEFINE_MUTEX(g_mfg_lock);

static int g_curFreqID;
static int g_init = 0;

static int pm_callback_power_on_nolock(struct kbase_device *kbdev)
{
	if (mtk_get_vgpu_power_on_flag() == MTK_VGPU_POWER_ON)
		return 0;

	mali_pr_debug("@%s: power on ...\n", __func__);

	if (g_init == 0)
		g_init = 1;
	else
		devfreq_resume_device(kbdev->devfreq);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x1 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Turn on GPU MTCMOS */
	mt_gpufreq_enable_MTCMOS(0);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x2 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* enable Clock Gating */
	mt_gpufreq_enable_CG();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x3 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	mt_gpufreq_external_cg_control();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x4 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
 #endif

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x5 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Resume frequency before power off */
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);
	/* mtk_set_mt_gpufreq_target(g_curFreqID); */

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x6 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	return 1;
}

static void pm_callback_power_off_nolock(struct kbase_device *kbdev)
{
	if (mtk_get_vgpu_power_on_flag() == MTK_VGPU_POWER_OFF)
		return;

	mali_pr_debug("@%s: power off ...\n", __func__);

	devfreq_suspend_device(kbdev->devfreq);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x7 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);
	g_curFreqID = mt_gpufreq_get_cur_freq_index();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x8 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	mt_gpufreq_check_bus_idle();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x9 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* disable Clock Gating */
	mt_gpufreq_disable_CG();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xA | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Turn off GPU MTCMOS */
	mt_gpufreq_disable_MTCMOS(0);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xB | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Turn off VGPU / VSRAM_GPU Buck */
	mt_gpufreq_voltage_enable_set(0);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xC | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	return;
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 0;

	mutex_lock(&g_mfg_lock);
	ret = pm_callback_power_on_nolock(kbdev);
	mutex_unlock(&g_mfg_lock);

	return ret;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	mutex_lock(&g_mfg_lock);
	pm_callback_power_off_nolock(kbdev);
	mutex_unlock(&g_mfg_lock);

	return;
}

void pm_callback_power_suspend(struct kbase_device *kbdev)
{
#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xD | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif
}

void pm_callback_power_resume(struct kbase_device *kbdev)
{
#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xE | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = pm_callback_power_suspend,
	.power_resume_callback = pm_callback_power_resume,
};

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
	.start = 0xFC010000,
	.end = 0xFC010000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */

static struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}

int mtk_platform_init(struct platform_device *pdev, struct kbase_device *kbdev)
{

	if (!pdev || !kbdev) {
		mali_pr_info("@%s: input parameter is NULL\n", __func__);
		return -1;
	}

	mali_pr_info("@%s: initialize successfully\n", __func__);

	return 0;
}
