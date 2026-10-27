// SPDX-License-Identifier: GPL-2.0
/*
 *file: panel-hx8394.c
 *autor: johnson.qiao
 *
 * WHEN		WHO		WHAT  WHERE  WHY
 *---------	----------	----------------
 *20210429      johnson		Create
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

struct hx8394 {
	struct drm_panel	panel;
	struct mipi_dsi_device	*dsi;

	struct backlight_device *backlight;
	struct regulator	*power;
	struct gpio_desc	*reset;
};

struct hx8394_cmd {
	u8 size;
	u8 data[128];
};


struct hx8394_cmd init_cmds[] = {
	{0x04, {0xB9, 0xFF, 0x83, 0x94}},
	{0x07, {0xBA, 0x63, 0x03, 0x68, 0x6b, 0xb2, 0xc0}},
	{0x0B, {0xB1, 0x50, 0x12, 0x72, 0x09, 0x33, 0x54, 0xB1, 0x31, 0x6B, 0x2F}},
	{0x07, {0xB2, 0x00, 0x80, 0x64, 0x0e, 0x0d, 0x2f}},
	{0x16, {0xB4, 0x73, 0x74, 0x73, 0x74, 0x73, 0x74, 0x01, 0x0C, 0x86, 0x75, 0x00, 0x3F, 0x73, 0x74, 0x73, 0x74, 0x73, 0x74, 0x01, 0x0C, 0x86}},
	{0x22, {0xD3, 0x00, 0x00, 0x07, 0x07, 0x40, 0x07, 0x10, 0x00, 0x08, 0x10, 0x08, 0x00, 0x08, 0x54, 0x15, 0x0e, 0x05, 0x0e, 0x02, 0x15, 0x06, 0x05, 0x06, 0x47, 0x44, 0x0a, 0x0a, 0x4b, 0x10, 0x07, 0x07, 0x0e, 0x40}},
	{0x2D, {0xD5, 0x1a, 0x1a, 0x1b, 0x1b, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x24, 0x25, 0x18, 0x18, 0x26, 0x27, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x20, 0x21, 0x18, 0x18, 0x18, 0x18}},
	{0x2D, {0xD6, 0x1a, 0x1a, 0x1b, 0x1b, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x21, 0x20, 0x18, 0x18, 0x27, 0x26, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x25, 0x24, 0x18, 0x18, 0x18, 0x18}},
	{0x3B, {0xE0, 0x00, 0x0D, 0x1B, 0x22, 0x25, 0x2A, 0x2F, 0x2C, 0x5A, 0x6B, 0x7A, 0x77, 0x7E, 0x8E, 0x92, 0x95, 0x9F, 0x9E, 0x99, 0xa1, 0xb0, 0x57, 0x55, 0x5C, 0x5F, 0x5F, 0x67, 0x6F, 0x7f, 0x00, 0x0D, 0x1B, 0x22, 0x25, 0x2A, 0x2F, 0x2C, 0x5A, 0x6B, 0x7A, 0x77, 0x7E, 0x8E, 0x92, 0x95, 0x9F, 0x9E, 0x99, 0xa1, 0xb0, 0x57, 0x55, 0x5C, 0x5F, 0x5F, 0x67, 0x6F, 0x7f}},
	{0x03, {0xC0, 0x1f, 0x31}},
	{0x02, {0xCC, 0x0B}},
	{0x03, {0xB6, 0x78, 0x78}},
	{0x02, {0xD4, 0x02}},
	{0x02, {0xBD, 0x02}},
	{0x0D, {0xD8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
	{0x02, {0xBD, 0x00}},
	{0x02, {0xBD, 0x01}},
	{0x02, {0xB1, 0x00}},
	{0x02, {0xBD, 0x00}},
	{0x08, {0xBF, 0x40, 0x81, 0x50, 0x00, 0x1A, 0xFC, 0x01}},
};

static inline struct hx8394 *panel_to_hx8394(struct drm_panel *panel)
{
	return container_of(panel, struct hx8394, panel);
}

static int hx8394_prepare(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);
	unsigned int i;
	int ret;

	/* Power the panel */
	ret = regulator_enable(ctx->power);
	if (ret)
		return ret;
	msleep(5);

	/* And reset it */
	gpiod_set_value(ctx->reset, 1);
	msleep(20);

	gpiod_set_value(ctx->reset, 0);
	msleep(20);
	gpiod_set_value(ctx->reset, 1);
	msleep(120);

	for (i = 0; i < ARRAY_SIZE(init_cmds); i++) {
		const struct hx8394_cmd *instr = &init_cmds[i];
		ret = mipi_dsi_dcs_write_buffer(ctx->dsi, instr->data, instr->size);
		if (ret < 0){
			pr_err("hx8394 write buffer fail %d\n", ret);
			return ret;
		}
	}

	ret = mipi_dsi_dcs_set_tear_on(ctx->dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_exit_sleep_mode(ctx->dsi);
	if (ret)
		return ret;

	msleep(120);

	mipi_dsi_dcs_set_display_on(ctx->dsi);

	return 0;
}

static int hx8394_enable(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);

	//msleep(120);

	//mipi_dsi_dcs_set_display_on(ctx->dsi);
	backlight_enable(ctx->backlight);

	return 0;
}

static int hx8394_disable(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);

	backlight_disable(ctx->backlight);
	return 0;
}

static int hx8394_unprepare(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);
	mipi_dsi_dcs_set_display_off(ctx->dsi);
	mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
	regulator_disable(ctx->power);
	//gpiod_set_value(ctx->reset, 1);

	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock		= 68278,
	.vrefresh	= 60,

	.hdisplay	= 720,
	.hsync_start	= 720 + 50,
	.hsync_end	= 720 + 50 + 50,
	.htotal		= 720 + 50 + 50 + 50,

	.vdisplay	= 1280,
	.vsync_start	= 1280 + 10,
	.vsync_end	= 1280 + 4 + 10,
	.vtotal		= 1280 + 4 + 10 + 4,
};

static int hx8394_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct hx8394 *ctx = panel_to_hx8394(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(&ctx->dsi->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay,
			default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	panel->connector->display_info.width_mm = mode->width_mm;
	panel->connector->display_info.height_mm = mode->height_mm;

	return 1;
}

static const struct drm_panel_funcs hx8394_funcs = {
	.prepare	= hx8394_prepare,
	.unprepare	= hx8394_unprepare,
	.enable		= hx8394_enable,
	.disable	= hx8394_disable,
	.get_modes	= hx8394_get_modes,
};

#ifdef HX8394_DEBUFG_FS //CONFIG_DEBUG_FS
static u8 regaddr = 0;

static int ili9881_control_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "enable\n"
				  "prepare\n"
				  "start\n");
	return 0;
}

static int ili9881_control_open(struct inode *inode, struct file *file)
{
	return single_open(file, ili9881_control_show, inode->i_private);
}

static ssize_t ili9881_control_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct hx8394 *ctx = s->private;
	char			buf[32] = {};

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "enable", 6)){
		//mode = DWC3_GCTL_PRTCAP_HOST;
		hx8394_enable(&ctx->panel);
	}
	else if (!strncmp(buf, "disable", 7)){
		hx8394_disable(&ctx->panel);
	}
	else if (!strncmp(buf, "start", 5)){
		hx8394_prepare(&ctx->panel);
	}

	return count;
}

static const struct file_operations ili9881_control_fops = {
	.open			= ili9881_control_open,
	.write			= ili9881_control_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int ili9881_reg_show(struct seq_file *s, void *unused)
{
	struct hx8394 *ctx = s->private;
	u8 rbuf[2] = {0};

	mipi_dsi_dcs_read(ctx->dsi, regaddr, rbuf, 1);
	seq_printf(s, "cmd=0x%02x, val=0x%02x\n", regaddr, rbuf[0]);
    
	return 0;
}

static int ili9881_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, ili9881_reg_show, inode->i_private);
}

static ssize_t ili9881_reg_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct hx8394 *ctx = s->private;
	u8			cmd = 0;
	char			buf[32] = {};

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;
	
	if (!strncmp(buf, "page", 4)){
		//mode = DWC3_GCTL_PRTCAP_HOST;
		sscanf((buf+4), "%x", &cmd);
		pr_info("ili9881 switch page %d\n", cmd);
		hx8394_switch_page(ctx, cmd);
	}
	else if (!strncmp(buf, "reg", 3)){
		sscanf((buf+3), "%x", &cmd);
		regaddr = cmd;
		pr_warn("ili9881 set regcmd = 0x%02x\n", cmd);
	} else if (!strncmp(buf, "write", 5)){
		u8 data;
		sscanf((buf+5), "%x %x", &cmd, &data);
		u8 buf[2] = {cmd, data};
		mipi_dsi_dcs_write_buffer(ctx->dsi, buf, sizeof(buf));
	}

	return count;
}


static const struct file_operations ili9881_reg_fops = {
	.open			= ili9881_reg_open,
	.write			= ili9881_reg_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

struct dentry *ili9881_root = NULL;
static int __init ili9881_debugfs_init(struct hx8394 *ctx)
{
	ili9881_root = debugfs_create_dir("dsi_panel", NULL);

	if (mtk_dbi_root){
		debugfs_create_file("control", S_IRUGO | S_IWUSR, ili9881_root, ctx,
			    	&ili9881_control_fops);
		debugfs_create_file("reg", S_IRUGO | S_IWUSR, ili9881_root, ctx,
			    	&ili9881_reg_fops);
	}
	return 0;
}
#endif

static int hx8394_dsi_probe(struct mipi_dsi_device *dsi)
{
	struct device_node *np;
	struct hx8394 *ctx;
	int ret;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	mipi_dsi_set_drvdata(dsi, ctx);
	ctx->dsi = dsi;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = &dsi->dev;
	ctx->panel.funcs = &hx8394_funcs;

	ctx->power = devm_regulator_get(&dsi->dev, "power");
	if (IS_ERR(ctx->power)) {
		dev_err(&dsi->dev, "Couldn't get our power regulator\n");
		return PTR_ERR(ctx->power);
	}

	ctx->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset)) {
		dev_err(&dsi->dev, "Couldn't get our reset GPIO\n");
		return PTR_ERR(ctx->reset);
	}

	//np = of_parse_phandle(dsi->dev.of_node, "backlight", 0);
	//if (np) {
	//	ctx->backlight = of_find_backlight_by_node(np);
	//	of_node_put(np);

	//	if (!ctx->backlight)
	//		return -EPROBE_DEFER;
	//}

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0)
		return ret;

	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	return mipi_dsi_attach(dsi);
}

static int hx8394_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct hx8394 *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	if (ctx->backlight)
		put_device(&ctx->backlight->dev);

	return 0;
}


static const struct of_device_id hx8394_of_match[] = {
	{ .compatible = "mediatek,hx8394" },
	{ }
};
MODULE_DEVICE_TABLE(of, hx8394_of_match);

static struct mipi_dsi_driver hx8394_dsi_driver = {
	.probe		= hx8394_dsi_probe,
	.remove		= hx8394_dsi_remove,
	.driver = {
		.name		= "hx8394-dsi",
		.of_match_table	= hx8394_of_match,
	},
};
module_mipi_dsi_driver(hx8394_dsi_driver);

MODULE_AUTHOR("johnson.qiao@quectel.com");
MODULE_DESCRIPTION("Hx8394 Controller Driver");
MODULE_LICENSE("GPL v2");

