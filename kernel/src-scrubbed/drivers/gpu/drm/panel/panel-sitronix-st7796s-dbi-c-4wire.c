/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>

#include <drm/drmP.h>
#include <drm/drm_panel.h>

#include "../mediatek/mediatek_v2/mtk_dbi.h"

#include <video/mipi_display.h>

//Switch to test different type
//#define TEST_DBI_B_8BITS
//#define TEST_DBI_B_9BITS
#define TEST_DBI_C          // Please define this to test DBI C 3Wire
#define TEST_DBI_C_4WIRE    // Define TEST_DBI_C and TEST_DBI_C_4WIRE for 4Wire

// TODO: Use to dynmiac detect LCD, if no exsited, return error
//#define DYNAMIC_DETECT_LCD

#define ST7796S_COLMOD_RGB_FMT_18BITS        (6 << 4)
#define ST7796S_COLMOD_CTRL_FMT_18BITS        (6 << 0)

#define ST7796S_RAMCTRL_CMD        0xb0
#define ST7796S_RAMCTRL_RM_RGB            BIT(4)
#define ST7796S_RAMCTRL_DM_RGB            BIT(0)
#define ST7796S_RAMCTRL_MAGIC            (3 << 6)
#define ST7796S_RAMCTRL_EPF(n)            (((n) & 3) << 4)

#define ST7796S_RGBCTRL_CMD        0xb1
#define ST7796S_RGBCTRL_WO            BIT(7)
#define ST7796S_RGBCTRL_RCM(n)            (((n) & 3) << 5)
#define ST7796S_RGBCTRL_VSYNC_HIGH        BIT(3)
#define ST7796S_RGBCTRL_HSYNC_HIGH        BIT(2)
#define ST7796S_RGBCTRL_PCLK_HIGH        BIT(1)
#define ST7796S_RGBCTRL_VBP(n)            ((n) & 0x7f)
#define ST7796S_RGBCTRL_HBP(n)            ((n) & 0x1f)

#define ST7796S_PORCTRL_CMD        0xb2
#define ST7796S_PORCTRL_IDLE_BP(n)        (((n) & 0xf) << 4)
#define ST7796S_PORCTRL_IDLE_FP(n)        ((n) & 0xf)
#define ST7796S_PORCTRL_PARTIAL_BP(n)        (((n) & 0xf) << 4)
#define ST7796S_PORCTRL_PARTIAL_FP(n)        ((n) & 0xf)

#define ST7796S_GCTRL_CMD        0xb7
#define ST7796S_GCTRL_VGHS(n)            (((n) & 7) << 4)
#define ST7796S_GCTRL_VGLS(n)            ((n) & 7)

#define ST7796S_VCOMS_CMD        0xbb

#define ST7796S_LCMCTRL_CMD        0xc0
#define ST7796S_LCMCTRL_XBGR            BIT(5)
#define ST7796S_LCMCTRL_XMX            BIT(3)
#define ST7796S_LCMCTRL_XMH            BIT(2)

#define ST7796S_VDVVRHEN_CMD        0xc2
#define ST7796S_VDVVRHEN_CMDEN            BIT(0)

#define ST7796S_VRHS_CMD        0xc3

#define ST7796S_VDVS_CMD        0xc4

#define ST7796S_FRCTRL2_CMD        0xc6

#define ST7796S_PWCTRL1_CMD        0xd0
#define ST7796S_PWCTRL1_MAGIC            0xa4
#define ST7796S_PWCTRL1_AVDD(n)            (((n) & 3) << 6)
#define ST7796S_PWCTRL1_AVCL(n)            (((n) & 3) << 4)
#define ST7796S_PWCTRL1_VDS(n)            ((n) & 3)

#define ST7796S_PVGAMCTRL_CMD        0xe0
#define ST7796S_PVGAMCTRL_JP0(n)        (((n) & 3) << 4)
#define ST7796S_PVGAMCTRL_JP1(n)        (((n) & 3) << 4)
#define ST7796S_PVGAMCTRL_VP0(n)        ((n) & 0xf)
#define ST7796S_PVGAMCTRL_VP1(n)        ((n) & 0x3f)
#define ST7796S_PVGAMCTRL_VP2(n)        ((n) & 0x3f)
#define ST7796S_PVGAMCTRL_VP4(n)        ((n) & 0x1f)
#define ST7796S_PVGAMCTRL_VP6(n)        ((n) & 0x1f)
#define ST7796S_PVGAMCTRL_VP13(n)        ((n) & 0xf)
#define ST7796S_PVGAMCTRL_VP20(n)        ((n) & 0x7f)
#define ST7796S_PVGAMCTRL_VP27(n)        ((n) & 7)
#define ST7796S_PVGAMCTRL_VP36(n)        (((n) & 7) << 4)
#define ST7796S_PVGAMCTRL_VP43(n)        ((n) & 0x7f)
#define ST7796S_PVGAMCTRL_VP50(n)        ((n) & 0xf)
#define ST7796S_PVGAMCTRL_VP57(n)        ((n) & 0x1f)
#define ST7796S_PVGAMCTRL_VP59(n)        ((n) & 0x1f)
#define ST7796S_PVGAMCTRL_VP61(n)        ((n) & 0x3f)
#define ST7796S_PVGAMCTRL_VP62(n)        ((n) & 0x3f)
#define ST7796S_PVGAMCTRL_VP63(n)        (((n) & 0xf) << 4)

#define ST7796S_NVGAMCTRL_CMD        0xe1
#define ST7796S_NVGAMCTRL_JN0(n)        (((n) & 3) << 4)
#define ST7796S_NVGAMCTRL_JN1(n)        (((n) & 3) << 4)
#define ST7796S_NVGAMCTRL_VN0(n)        ((n) & 0xf)
#define ST7796S_NVGAMCTRL_VN1(n)        ((n) & 0x3f)
#define ST7796S_NVGAMCTRL_VN2(n)        ((n) & 0x3f)
#define ST7796S_NVGAMCTRL_VN4(n)        ((n) & 0x1f)
#define ST7796S_NVGAMCTRL_VN6(n)        ((n) & 0x1f)
#define ST7796S_NVGAMCTRL_VN13(n)        ((n) & 0xf)
#define ST7796S_NVGAMCTRL_VN20(n)        ((n) & 0x7f)
#define ST7796S_NVGAMCTRL_VN27(n)        ((n) & 7)
#define ST7796S_NVGAMCTRL_VN36(n)        (((n) & 7) << 4)
#define ST7796S_NVGAMCTRL_VN43(n)        ((n) & 0x7f)
#define ST7796S_NVGAMCTRL_VN50(n)        ((n) & 0xf)
#define ST7796S_NVGAMCTRL_VN57(n)        ((n) & 0x1f)
#define ST7796S_NVGAMCTRL_VN59(n)        ((n) & 0x1f)
#define ST7796S_NVGAMCTRL_VN61(n)        ((n) & 0x3f)
#define ST7796S_NVGAMCTRL_VN62(n)        ((n) & 0x3f)
#define ST7796S_NVGAMCTRL_VN63(n)        (((n) & 0xf) << 4)

struct st7796s {
	bool prepared;
	struct drm_panel panel;
	//struct spi_device *spi;
	struct device *dev;
	struct gpio_desc *reset;
	struct backlight_device *backlight;
	struct regulator *power;
	struct gpio_desc *bias;
};

enum st7796s_prefix {
	ST7796S_COMMAND = 0,
	ST7796S_DATA = 1,
};

const struct mipi_dbi_host_ops *ops;

static inline struct st7796s *panel_to_st7796s(struct drm_panel *panel)
{
	return container_of(panel, struct st7796s, panel);
}

static void st7796s_write_command(struct st7796s *ctx, u8 cmd)
{
	ops->send_cmd(&ctx->panel, cmd);
}

static void st7796s_write_data(struct st7796s *ctx, u8 data)
{
	ops->send_data(&ctx->panel, data);
}

#ifdef DYNAMIC_DETECT_LCD
static unsigned int st7796s_read_data(struct st7796s *ctx)
{
	return ops->read_data(&ctx->panel);
}
#endif

static const struct drm_display_mode default_mode = {
	.clock = 6815, // Pixel Clock (kHz): htotal * vtotal * vrefresh / 1000
	.hdisplay = 320,
	.hsync_start = 320 + 38,      // HFP
	.hsync_end = 320 + 38 + 50,   // HSA = HPW
	.htotal = 320 + 38 + 50 + 50, // HBP
	.vdisplay = 480,
	.vsync_start = 480 + 8,       // VFP
	.vsync_end = 480 + 8 + 4,     // VSA
	.vtotal = 480 + 8 + 4 + 4,    // VBP
	.vrefresh = 30,
};

static int st7796s_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	panel->connector->display_info.width_mm = 61;
	panel->connector->display_info.height_mm = 103;

	return 1;
}

static int st7796s_prepare(struct drm_panel *panel)
{
	struct st7796s *ctx = panel_to_st7796s(panel);
	int ret;
#ifdef DYNAMIC_DETECT_LCD
	unsigned int da, db, dc;
	unsigned int d1, d2, d3, d4;
#endif

	dev_info(ctx->panel.dev, "[%s] enter\n", __func__);

	if (ctx->prepared) {
		// LK already init LCM, only need to exit sleep mode and enable TE if necessary
		st7796s_write_command(ctx, MIPI_DCS_EXIT_SLEEP_MODE);

		/* We need to wait 120ms after a sleep out command */
		msleep(120);

		// Set TE line
		st7796s_write_command(ctx, 0x35);
		st7796s_write_data(ctx, 0x00);	// vblank only

		// Set Interface Pixel Format
		st7796s_write_command(ctx, MIPI_DCS_SET_PIXEL_FORMAT);
		st7796s_write_data(ctx, 0x55);  //16bits

		dev_info(ctx->panel.dev, "[%s] already prepared, exit\n", __func__);
		return 0;
	}

	ret = regulator_enable(ctx->power);
	if (ret)
		return ret;

	dev_info(ctx->panel.dev, "[%s] set gpio reset start\n", __func__);

	ctx->reset = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset)) {
		dev_err(ctx->dev, "Couldn't get our reset line\n");
		return PTR_ERR(ctx->reset);
	}

	gpiod_set_value(ctx->reset, 1);
	msleep(20);
	gpiod_set_value(ctx->reset, 0);
	msleep(20);
	gpiod_set_value(ctx->reset, 1);
	msleep(20);
	devm_gpiod_put(ctx->dev, ctx->reset);

	// Reset LCM
	st7796s_write_command(ctx, 0x01);
	msleep(50);

	dev_info(ctx->panel.dev, "[%s] set gpio reset done\n", __func__);

	st7796s_write_command(ctx, MIPI_DCS_EXIT_SLEEP_MODE);

	/* We need to wait 120ms after a sleep out command */
	msleep(120);

#ifdef DYNAMIC_DETECT_LCD
	// Get DDIC ID to check if Panel existed
	// Read DA
	st7796s_write_command(ctx, 0xDA);
	d1 = st7796s_read_data(ctx);
	da = st7796s_read_data(ctx);
	dev_info(ctx->panel.dev, "[%s] read DAh (%02X, %02X)\n", __func__, d1, da);

	// Read DB
	st7796s_write_command(ctx, 0xDB);
	d1 = st7796s_read_data(ctx);
	db = st7796s_read_data(ctx);
	dev_info(ctx->panel.dev, "[%s] read DBh (%02X, %02X)\n", __func__, d1, db);

	// Read DC
	st7796s_write_command(ctx, 0xDC);
	d1 = st7796s_read_data(ctx);
	dc = st7796s_read_data(ctx);
	dev_info(ctx->panel.dev, "[%s] read DCh (%02X, %02X)\n", __func__, d1, dc);

	// Read DDIC ID
	st7796s_write_command(ctx, 0x04);
	d1 = st7796s_read_data(ctx);
	d2 = st7796s_read_data(ctx);
	d3 = st7796s_read_data(ctx);
	d4 = st7796s_read_data(ctx);
	dev_info(ctx->panel.dev, "[%s] read DDIC ID (%02X, %02X, %02X, %02X)\n",
			__func__, d1, d2, d3, d4);

	if ((da == 0 && db == 0 && dc == 0) || da != d2 || db != d3 || dc != d4) {
		// Means there is no LCD detect, return error
		return -1;
	}
#endif /*DYNAMIC_DETECT_LCD*/

	st7796s_write_command(ctx, MIPI_DCS_SET_ADDRESS_MODE);
	st7796s_write_data(ctx, 0x44); // RGB order

	// Set Interface Pixel Format
	st7796s_write_command(ctx, MIPI_DCS_SET_PIXEL_FORMAT);
	st7796s_write_data(ctx, 0x55);  //16bits

	st7796s_write_command(ctx, 0xF0);
	st7796s_write_data(ctx, 0xC3);

	st7796s_write_command(ctx, 0xF0);
	st7796s_write_data(ctx, 0x96);

	// Set TE line
	st7796s_write_command(ctx, 0x35);
	st7796s_write_data(ctx, 0x00);  // vblank only

#ifdef TEST_DBI_C
	st7796s_write_command(ctx, ST7796S_RAMCTRL_CMD);
	st7796s_write_data(ctx, 0x80);
#endif

	st7796s_write_command(ctx, 0xB4);
	st7796s_write_data(ctx, 0x01);

	st7796s_write_command(ctx, 0xB5);
	st7796s_write_data(ctx, 0x02); //RF Modify
	st7796s_write_data(ctx, 0x02); //RF Modify
	st7796s_write_data(ctx, 0x00);
	st7796s_write_data(ctx, 0x04); //RF Modify

#ifdef TEST_DBI_C
#else /* DBI-B only */
	st7796s_write_command(ctx, 0xB6);   // DBI-B only
	st7796s_write_data(ctx, 0x8A);
	st7796s_write_data(ctx, 0x07);
	st7796s_write_data(ctx, 0x3B);

	st7796s_write_command(ctx, 0xB7);   // DBI-B only
	st7796s_write_data(ctx, 0xC6);
#endif

	st7796s_write_command(ctx, 0xE8);
	st7796s_write_data(ctx, 0x40);
	st7796s_write_data(ctx, 0x8A);
	st7796s_write_data(ctx, 0x00);
	st7796s_write_data(ctx, 0x00);
	st7796s_write_data(ctx, 0x29);
	st7796s_write_data(ctx, 0x0F); //RF
	st7796s_write_data(ctx, 0x32); //RF
	st7796s_write_data(ctx, 0x33);

	st7796s_write_command(ctx, 0xC0);
	st7796s_write_data(ctx, 0x80);
	st7796s_write_data(ctx, 0x31);   //VGH=14.968V, VGL=-11.385

	st7796s_write_command(ctx, 0xC1);   //GVDD
	st7796s_write_data(ctx, 0x18);   //4.75V

	st7796s_write_command(ctx, 0xC2);
	st7796s_write_data(ctx, 0xA7);

	st7796s_write_command(ctx, 0xC5);   //VCOM
	st7796s_write_data(ctx, 0x10);   //0.725V

	st7796s_write_command(ctx, 0xE0);
	st7796s_write_data(ctx, 0x78);
	st7796s_write_data(ctx, 0x0C);
	st7796s_write_data(ctx, 0x17);
	st7796s_write_data(ctx, 0x11);
	st7796s_write_data(ctx, 0x11);
	st7796s_write_data(ctx, 0x1B);
	st7796s_write_data(ctx, 0x3D);
	st7796s_write_data(ctx, 0x44);
	st7796s_write_data(ctx, 0x4C);
	st7796s_write_data(ctx, 0x38);
	st7796s_write_data(ctx, 0x12);
	st7796s_write_data(ctx, 0x10);
	st7796s_write_data(ctx, 0x1C);
	st7796s_write_data(ctx, 0x1F);

	st7796s_write_command(ctx, 0xE1);
	st7796s_write_data(ctx, 0xF0);
	st7796s_write_data(ctx, 0x06);
	st7796s_write_data(ctx, 0x11);
	st7796s_write_data(ctx, 0x0C);
	st7796s_write_data(ctx, 0x0E);
	st7796s_write_data(ctx, 0x19);
	st7796s_write_data(ctx, 0x3B);
	st7796s_write_data(ctx, 0x44);
	st7796s_write_data(ctx, 0x4E);
	st7796s_write_data(ctx, 0x3B);
	st7796s_write_data(ctx, 0x16);
	st7796s_write_data(ctx, 0x16);
	st7796s_write_data(ctx, 0x22);
	st7796s_write_data(ctx, 0x26);

	st7796s_write_command(ctx, 0xF0);
	st7796s_write_data(ctx, 0x3C);

	st7796s_write_command(ctx, 0xF0);
	st7796s_write_data(ctx, 0x69);

	msleep(120);

	//msleep(10);
	//st7796s_write_command(ctx, 0x2C);

	st7796s_write_command(ctx, 0x2A);
	st7796s_write_data(ctx, 0x00);
	st7796s_write_data(ctx, 0x00);
	st7796s_write_data(ctx, 0x01);
	st7796s_write_data(ctx, 0x3F);

	st7796s_write_command(ctx, 0x2B);
	st7796s_write_data(ctx, 0x00);
	st7796s_write_data(ctx, 0x00);
	st7796s_write_data(ctx, 0x01);
	st7796s_write_data(ctx, 0xDF);

	//st7796s_write_command(ctx, MIPI_DCS_SET_DISPLAY_ON);

	// add for testing start
	/* We need to wait 120ms after a sleep out command */
	//msleep(10);
	//st7796s_write_command(ctx, 0x2C);
	// add for testing end

	ctx->prepared = true;

	dev_info(ctx->panel.dev, "[%s] exit prepared=%d\n", __func__, ctx->prepared);

	return 0;
}

static int st7796s_enable(struct drm_panel *panel)
{
	struct st7796s *ctx = panel_to_st7796s(panel);

	dev_info(ctx->panel.dev, "[%s] enter\n", __func__);

	if (ctx->backlight) {
		ctx->backlight->props.state &= ~BL_CORE_FBBLANK;
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	msleep(120);
	st7796s_write_command(ctx, MIPI_DCS_SET_DISPLAY_ON);
	msleep(20);

	dev_info(ctx->panel.dev, "[%s] exit\n", __func__);

	return 0;
}

static int st7796s_disable(struct drm_panel *panel)
{
	struct st7796s *ctx = panel_to_st7796s(panel);

	dev_info(ctx->panel.dev, "[%s] enter\n", __func__);

	st7796s_write_command(ctx, MIPI_DCS_SET_DISPLAY_OFF);

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		ctx->backlight->props.state |= BL_CORE_FBBLANK;
		backlight_update_status(ctx->backlight);
	}

	dev_info(ctx->panel.dev, "[%s] exit\n", __func__);

	return 0;
}

static int st7796s_unprepare(struct drm_panel *panel)
{
	struct st7796s *ctx = panel_to_st7796s(panel);

	dev_info(ctx->panel.dev, "[%s] enter\n", __func__);

	st7796s_write_command(ctx, MIPI_DCS_ENTER_SLEEP_MODE);

	regulator_disable(ctx->power);

	ctx->prepared = false;

	dev_info(ctx->panel.dev, "[%s] exit\n", __func__);

	return 0;
}

static const struct drm_panel_funcs st7796s_drm_funcs = {
	.disable	= st7796s_disable,
	.enable		= st7796s_enable,
	.get_modes	= st7796s_get_modes,
	.prepare	= st7796s_prepare,
	.unprepare	= st7796s_unprepare,
};

static int st7796s_probe(struct device *dev)
{
	struct device_node *backlight;
	struct st7796s *ctx;
	struct mipi_dbi_device lcm_param;
	int ret;

	dev_info(dev, "st7796s_probe\n");

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

#ifdef TEST_DBI_C
	lcm_param.ctrl_type = MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI;
#else /* DBI-B */
	lcm_param.ctrl_type = MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI;
#endif

#ifdef TEST_DBI_C
	lcm_param.clk_freq = MTK_DBI_LCM_CLOCK_FREQ_156M;
	lcm_param.data_width = MTK_DBI_LCM_DATA_WIDTH_8BITS;
	lcm_param.color_format = MTK_DBI_LCM_COLOR_FORMAT_RGB565;
#elif defined (TEST_DBI_B_9BITS)
	lcm_param.clk_freq = MTK_DBI_LCM_CLOCK_FREQ_156M;
	lcm_param.data_width = MTK_DBI_LCM_DATA_WIDTH_9BITS;
	lcm_param.color_format = MTK_DBI_LCM_COLOR_FORMAT_RGB666;
#else
	lcm_param.clk_freq = MTK_DBI_LCM_CLOCK_FREQ_156M;
	lcm_param.data_width = MTK_DBI_LCM_DATA_WIDTH_8BITS;
	lcm_param.color_format = MTK_DBI_LCM_COLOR_FORMAT_RGB666;
#endif
	lcm_param.driving_current.group1 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	lcm_param.driving_current.group2 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	lcm_param.driving_current.group3 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	lcm_param.color_order = MTK_DBI_LCM_COLOR_ORDER_RGB;
	lcm_param.te_control.software_te = 0;
	lcm_param.te_control.te_repeat = 0;
	lcm_param.te_control.sync_mode = 0;
	lcm_param.te_control.te_edge = 0;
	lcm_param.te_control.te_enable = 1; //Set enable

#ifdef TEST_DBI_C
	lcm_param.dbi_c_params.wire_num = MTK_DBI_LCM_INTERFACE_3WIRE;

#ifdef TEST_DBI_C_4WIRE
	lcm_param.dbi_c_params.wire_num = MTK_DBI_LCM_INTERFACE_4WIRE;
#endif

	lcm_param.dbi_c_params.rw_timing.cs_setup_time = 0;
	lcm_param.dbi_c_params.rw_timing.cs_hold_time = 0;
	lcm_param.dbi_c_params.rw_timing.read_1st = 9;
	lcm_param.dbi_c_params.rw_timing.read_2nd = 9;
	lcm_param.dbi_c_params.rw_timing.write_1st = 1;
	lcm_param.dbi_c_params.rw_timing.write_2nd = 1;
	lcm_param.dbi_c_params.rw_timing.lsck_default = 1;
	lcm_param.dbi_c_params.rw_timing.lsck_1st_pol = 0;
	lcm_param.dbi_c_params.rw_timing.slow_down = 0;
	lcm_param.dbi_c_params.rw_timing.read_data_from_sdi = 0;

#else /* DBI-B */
#ifdef TEST_DBI_B_8BITS
	lcm_param.dbi_b_params.itf_size = MTK_DBI_LCM_INTERFACE_SIZE_8BITS;
#else   /* DBI-B 9 bits */
	lcm_param.dbi_b_params.itf_size = MTK_DBI_LCM_INTERFACE_SIZE_9BITS;
#endif
#ifdef DYNAMIC_DETECT_LCD
	lcm_param.dbi_b_params.rw_timing.cs_to_read_hold_time = 6;
	lcm_param.dbi_b_params.rw_timing.cs_to_read_setup_time = 6;
	lcm_param.dbi_b_params.rw_timing.read_latency_time = 0xb;
#else
	lcm_param.dbi_b_params.rw_timing.cs_to_read_hold_time = 1;
	lcm_param.dbi_b_params.rw_timing.cs_to_read_setup_time = 1;
	lcm_param.dbi_b_params.rw_timing.read_latency_time = 4;
#endif
	lcm_param.dbi_b_params.rw_timing.cs_to_write_hold_time = 1;
	lcm_param.dbi_b_params.rw_timing.cs_to_write_setup_time = 1;
	lcm_param.dbi_b_params.rw_timing.write_wait_state_time = 3;
	lcm_param.dbi_b_params.rw_timing.cs_remain_high_time = 0;
#endif

	//spi_set_drvdata(spi, ctx);
	ctx->dev = dev;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &st7796s_drm_funcs;

	dev_info(dev, "st7796s_probe2\n");

	ctx->power = devm_regulator_get(dev, "power");
	if (IS_ERR(ctx->power))
		return PTR_ERR(ctx->power);

	/*
	ctx->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset)) {
		dev_err(dev, "Couldn't get our reset line\n");
		return PTR_ERR(ctx->reset);
	}
	*/

	ctx->bias = devm_gpiod_get(dev, "bias", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->bias)) {
		dev_err(dev, "Couldn't get our bias line\n");
		return PTR_ERR(ctx->bias);
	}

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

	dev_set_drvdata(dev, ctx);

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0)
		goto err_free_backlight;

	mtk_dbi_host_attach(&ctx->panel, &lcm_param, &ops);

	dev_info(dev, "st7796s_probe ops=%p\n", ops);
	dev_info(dev, "st7796s_probe ops->send_cmd=%p\n", ops->send_cmd);

	// LK already init LCM
	ctx->prepared = true;

	return 0;

err_free_backlight:
	if (ctx->backlight)
		put_device(&ctx->backlight->dev);

	return ret;
}

static int st7796s_remove(struct device *dev)
{
	struct st7796s *ctx = dev_get_drvdata(dev);

	drm_panel_remove(&ctx->panel);

	if (ctx->backlight)
		put_device(&ctx->backlight->dev);

	return 0;
}


static const struct of_device_id st7796s_of_match[] = {
	{ .compatible = "sitronix,st7796s" },
	{ }
};
MODULE_DEVICE_TABLE(of, st7796s_of_match);

static int st7796s_driver_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;

	id = of_match_node(st7796s_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	return st7796s_probe(&pdev->dev);
}

static int st7796s_driver_platform_remove(struct platform_device *pdev)
{
	return st7796s_remove(&pdev->dev);
}

static struct platform_driver st7796s_driver_platform_driver = {
	.driver = {
		.name = "sitronix,st7796s",
		.of_match_table = st7796s_of_match,
	},
	.probe = st7796s_driver_platform_probe,
	.remove = st7796s_driver_platform_remove,
	//.shutdown = panel_simple_platform_shutdown,
};

static int __init st7796s_driver_init(void)
{
	int err;

	err = platform_driver_register(&st7796s_driver_platform_driver);
	if (err < 0)
		return err;

	return 0;
}
module_init(st7796s_driver_init);

static void __exit st7796s_driver_exit(void)
{
	platform_driver_unregister(&st7796s_driver_platform_driver);
}
module_exit(st7796s_driver_exit);

MODULE_DESCRIPTION("Sitronix st7796s LCD Driver");
MODULE_LICENSE("GPL v2");
