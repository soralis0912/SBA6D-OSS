/* 
 * file: panel-jd9851-dbi-c-4wire.c
 * author: johnson qiao
 * 
 * WHEN    	WHO       	WHAT  WHERE   WHY
 *----------	------------	------------
 *2021.04.25    johnson.qiao	Create
 */

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>

#include <drm/drmP.h>
#include <drm/drm_panel.h>

#include "../mediatek/mediatek_v2/mtk_dbi.h"

#include <video/mipi_display.h>

#define TEST_DBI_C          // Please define this to test DBI C 3Wire
#define TEST_DBI_C_4WIRE    // If you would like to test 4WIRE, please define
                            // TEST_DBI_C and TEST_DBI_C_4WIRE at the same time


struct jd9851 {
    bool prepared;
    struct drm_panel panel;
    struct device *dev;
    struct gpio_desc *reset;
    struct backlight_device *backlight;
    struct regulator *power;
    struct gpio_desc *bias;
};

enum jd9851_prefix {
    JD9851COMMAND = 0,
    JD9851DATA = 1,
};

typedef struct spi_lcd_cmd_struct{
        unsigned char reg_addr;
        unsigned char len;
        int delay_ms;
}spi_lcd_cmd_t;

const struct mipi_dbi_host_ops *ops;


spi_lcd_cmd_t lcd_cmds[]={
		{0xDF,3,0},
		{0xDE,1,0},
		{0XB7,4,0},
		{0xC8,32,0},
		{0xB9,3,0},
		{0xBB,8,0},
		{0xBC,2,0},
		{0xC0,2,0},
		{0xC1,1,0},
		{0xC3,9,0},
		{0xC4,17,0},
		{0xD0,6,0},
		{0xD7,2,0},
		{0xDE,1,1},
		{0xB8,5,0},
		{0xC1,4,0},
		{0xDE,1,0},
		{0xDE,1,1},
		{0xC5,1,1},
		{0xCA,3,1},
		{0xDE,1,0},
		{0x36,1,0},
		{0x3A,1,0},
		{0x11,0,120},
		//{0x29,0,120},

};

unsigned char lcd_datas[] = {
	0x98, 0x51, 0xE9,    //command: 0xC8
	0x00,
	0x0A,0x51,0x0A,0x1F,
	0x3F,0x37,0x32,0x2F,0x30,0x36,0x31,0x30,0x2E,0x2D,0x27,0x17,0x11,0x08,0x02,0x04,0x3F,0x37,0x31,0x2F,0x2F,0x36,0x31,0x30,0x2E,0x2C,0x27,0x17,0x11,0x08,0x02,0x04,
	0x33,0x28,0xCC,
	0x46,0x7A,0x40,0x40,0x6C,0x60,0x60,0x70,
	0x38,0x3C,
	0x31,0x20,
	0x12,
	0x08,0x00,0x0A,0x10,0x08,0x54,0x45,0x71,0x2C,
	0x00,0xA0,0x79,0x0E,0x0A,0x16,0x79,0x0E,0x0A,0x16,0x79,0x0E,0x0A,0x16,0x82,0x00,0x03,
	0x04,0x0C,0x6B,0x0F,0x07,0x03,
	0x00,0x00,
	0x02,
	0x19,0xA0,0x2F,0x04,0x33,
	0x10,0x66,0x66,0x01,
	0x00,
	0x02,
	0x01,
	0x10,0x20,0xF4,
	0x00,
	0x00,
	0x05,

};

static inline struct jd9851 *panel_to_jd9851(struct drm_panel *panel)
{
    return container_of(panel, struct jd9851, panel);
}

static void jd9851_write_command(struct jd9851 *ctx, u8 cmd)
{
    ops->send_cmd(&ctx->panel, cmd);
}

static void jd9851_write_data(struct jd9851 *ctx, u8 data)
{
    ops->send_data(&ctx->panel, data);
}

static const struct drm_display_mode default_mode = {
    .clock = 7000,
	.hdisplay = 240,
	.hsync_start = 240 + 38,
	.hsync_end = 240 + 38 + 10,
	.htotal = 240 + 38 + 10 + 10,
	.vdisplay = 320,
	.vsync_start = 320 + 8,
	.vsync_end = 320 + 8 + 4,
	.vtotal = 320 + 8 + 4 + 4,
	.vrefresh = 60,
};

static int jd9851_get_modes(struct drm_panel *panel)
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

#if 1
static void spi_lcd_set_addr(struct jd9851 *ctx, u16 x1, u16 y1, u16 x2, u16 y2)
{
	jd9851_write_command (ctx, 0x2A); 
	jd9851_write_data(ctx, x1>>8);
	jd9851_write_data(ctx, x1&0xFF);
	jd9851_write_data(ctx, x2>>8);
	jd9851_write_data(ctx, x2&0xFF);

	jd9851_write_command (ctx, 0x2B); 
	jd9851_write_data(ctx, y1>>8);
	jd9851_write_data(ctx, y1&0xFF);
	jd9851_write_data(ctx, y2>>8);
	jd9851_write_data(ctx, y2&0xFF);

	jd9851_write_command(ctx, 0x2c);
}

#define LCD_COLOR_WHITE  0xFFFF
#define LCD_COLOR_BLUE  0x001F
#define LCD_COLOR_GREEN  0x07E0
#define LCD_COLOR_RED   0xF800
#define LCD_COLOR_BLACK  0x0000

#define LCD_COLOR_NUM 5
static unsigned short color_bar[LCD_COLOR_NUM] = {LCD_COLOR_WHITE,LCD_COLOR_BLUE,LCD_COLOR_GREEN,LCD_COLOR_RED,LCD_COLOR_BLACK};
static void show_color_bar(struct jd9851 *ctx)
{
	int line_per_color = 320/LCD_COLOR_NUM;
	int pixel_per_color = 240*line_per_color;
	int i,j;

	spi_lcd_set_addr(ctx, 0, 0, 239,319);
	for(i = 0; i < LCD_COLOR_NUM; i++){
		for (j = 0; j < pixel_per_color; j++){
			jd9851_write_data(ctx, color_bar[i]>>8);
			jd9851_write_data(ctx, color_bar[i]&0xFF);
		}
	}
	jd9851_write_command(ctx, 0x29);
}

static void black_screen(struct jd9851 *ctx)
{
	int memsize = 240*320*16/8;
	int i;

	spi_lcd_set_addr(ctx, 0, 0, 239, 319);
	for (i = 0; i < memsize; i++){
		jd9851_write_data(ctx, 0x00);
	}
	//jd9851_write_command(ctx, 0x29);
}

#endif

static int jd9851_prepare(struct drm_panel *panel)
{
    struct jd9851 *ctx = panel_to_jd9851(panel);
    int ret;
    int i,j,n=0;

    printk(KERN_WARNING "[%s] enter \n", __func__);

    if (ctx->prepared) {
        printk(KERN_WARNING "[%s] already prepared, exit \n", __func__);
        return 0;
    }

    ret = regulator_enable(ctx->power);
    if (ret)
        return ret;

    printk(KERN_WARNING "[%s] set gpio reset start \n", __func__);

    gpiod_set_value(ctx->reset, 1);
    msleep(10);
    gpiod_set_value(ctx->reset, 0);
    msleep(10);
    gpiod_set_value(ctx->reset, 1);
    msleep(20);

    for (i=0; i<ARRAY_SIZE(lcd_cmds); i++) 
    {
	jd9851_write_command(ctx, lcd_cmds[i].reg_addr);
	for (j = 0; j < lcd_cmds[i].len; j++)
	{
	    jd9851_write_data(ctx, lcd_datas[n++]);			
	}
	if (lcd_cmds[i].delay_ms)
	{
		mdelay(lcd_cmds[i].delay_ms);
	}            
    }

    ctx->prepared = true;
    
    black_screen(ctx);
    //just for debug
    //show_color_bar(ctx);

    printk(KERN_DEBUG "[%s] exit prepared=%d\n", __func__, ctx->prepared);

    return 0;
}

static int jd9851_enable(struct drm_panel *panel)
{
    struct jd9851 *ctx = panel_to_jd9851(panel);

    printk(KERN_DEBUG "[%s] enter \n", __func__);

    if (ctx->backlight) {
        ctx->backlight->props.state &= ~BL_CORE_FBBLANK;
        ctx->backlight->props.power = FB_BLANK_UNBLANK;
        backlight_update_status(ctx->backlight);
    }

    msleep(120);
    jd9851_write_command(ctx, MIPI_DCS_SET_DISPLAY_ON);
    msleep(10);

    printk(KERN_DEBUG "[%s] exit \n", __func__);

    return 0;
}

static int jd9851_disable(struct drm_panel *panel)
{
    struct jd9851 *ctx = panel_to_jd9851(panel);

    printk(KERN_DEBUG "[%s] enter \n", __func__);

    jd9851_write_command(ctx, MIPI_DCS_SET_DISPLAY_OFF);

    if (ctx->backlight) {
        ctx->backlight->props.power = FB_BLANK_POWERDOWN;
        ctx->backlight->props.state |= BL_CORE_FBBLANK;
        backlight_update_status(ctx->backlight);
    }

    printk(KERN_DEBUG "[%s] exit \n", __func__);

    return 0;
}

static int jd9851_unprepare(struct drm_panel *panel)
{
    struct jd9851 *ctx = panel_to_jd9851(panel);

    printk(KERN_DEBUG "[%s] enter \n", __func__);

    jd9851_write_command(ctx, MIPI_DCS_ENTER_SLEEP_MODE);

    regulator_disable(ctx->power);

    ctx->prepared = false;

    printk(KERN_DEBUG "[%s] exit \n", __func__);

    return 0;
}

static const struct drm_panel_funcs jd9851_drm_funcs = {
    .disable    = jd9851_disable,
    .enable        = jd9851_enable,
    .get_modes    = jd9851_get_modes,
    .prepare    = jd9851_prepare,
    .unprepare    = jd9851_unprepare,
};

static int jd9851_probe(struct device *dev)
{
    struct device_node *backlight;
    struct jd9851 *ctx;
    struct mipi_dbi_device lcm_param;
    int ret;

    printk(KERN_WARNING "jd9851_probe\n");

    ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

#ifdef TEST_DBI_C
    lcm_param.ctrl_type = MTK_DBI_LCM_CTRL_TYPE_SERIAL_DBI; //testing
#else /* DBI-B */
    lcm_param.ctrl_type = MTK_DBI_LCM_CTRL_TYPE_PARALLEL_DBI; //testing
#endif

#ifdef TEST_DBI_C
    lcm_param.clk_freq = MTK_DBI_LCM_CLOCK_FREQ_104M;
    lcm_param.data_width = MTK_DBI_LCM_DATA_WIDTH_8BITS;
    lcm_param.color_format = MTK_DBI_LCM_COLOR_FORMAT_RGB565;
#elif defined (TEST_DBI_B_9BITS)
    lcm_param.clk_freq = MTK_DBI_LCM_CLOCK_FREQ_104M;
    lcm_param.data_width = MTK_DBI_LCM_DATA_WIDTH_9BITS;
    lcm_param.color_format = MTK_DBI_LCM_COLOR_FORMAT_RGB565;
#else
    lcm_param.clk_freq = MTK_DBI_LCM_CLOCK_FREQ_104M;
    lcm_param.data_width = MTK_DBI_LCM_DATA_WIDTH_8BITS;
    lcm_param.color_format = MTK_DBI_LCM_COLOR_FORMAT_RGB565;
#endif
	lcm_param.driving_current.group1 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	lcm_param.driving_current.group2 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;
	lcm_param.driving_current.group3 = MTK_DBI_LCM_DRIVING_CURRENT_6MA;

    lcm_param.color_order = MTK_DBI_LCM_COLOR_ORDER_RGB;
    lcm_param.te_control.software_te = 0;
    lcm_param.te_control.te_repeat = 0;
    lcm_param.te_control.sync_mode = 0;
    lcm_param.te_control.te_edge = 0;
    lcm_param.te_control.te_enable = 0; //Set disable

#ifdef TEST_DBI_C
    lcm_param.dbi_c_params.wire_num = MTK_DBI_LCM_INTERFACE_3WIRE;

#ifdef TEST_DBI_C_4WIRE
    lcm_param.dbi_c_params.wire_num = MTK_DBI_LCM_INTERFACE_4WIRE;
#endif

    lcm_param.dbi_c_params.rw_timing.cs_setup_time = 0;
    lcm_param.dbi_c_params.rw_timing.cs_hold_time = 0;
    lcm_param.dbi_c_params.rw_timing.read_1st = 7;
    lcm_param.dbi_c_params.rw_timing.read_2nd = 7;
    lcm_param.dbi_c_params.rw_timing.write_1st= 0;
    lcm_param.dbi_c_params.rw_timing.write_2nd= 0;

#else /* DBI-B */
#ifdef TEST_DBI_B_8BITS
    lcm_param.dbi_b_params.itf_size = MTK_DBI_LCM_INTERFACE_SIZE_8BITS;
#else   /* DBI-B 9 bits */
    lcm_param.dbi_b_params.itf_size = MTK_DBI_LCM_INTERFACE_SIZE_9BITS;
#endif
    lcm_param.dbi_b_params.rw_timing.cs_to_read_hold_time = 1;
    lcm_param.dbi_b_params.rw_timing.cs_to_read_setup_time = 1;
    lcm_param.dbi_b_params.rw_timing.read_latency_time = 4;
    lcm_param.dbi_b_params.rw_timing.cs_to_write_hold_time = 1;
    lcm_param.dbi_b_params.rw_timing.cs_to_write_setup_time = 1;
    lcm_param.dbi_b_params.rw_timing.write_wait_state_time = 5;
#endif

    ctx->dev = dev;

    drm_panel_init(&ctx->panel);
    ctx->panel.dev = dev;
    ctx->panel.funcs = &jd9851_drm_funcs;

    ctx->power = devm_regulator_get(dev, "power");
    if (IS_ERR(ctx->power))
        return PTR_ERR(ctx->power);

    ctx->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
    if (IS_ERR(ctx->reset)) {
        dev_err(dev, "Couldn't get our reset line\n");
        return PTR_ERR(ctx->reset);
    }

#if 0
    backlight = of_parse_phandle(dev->of_node, "backlight", 0);
    if (backlight) {
        ctx->backlight = of_find_backlight_by_node(backlight);
        of_node_put(backlight);

        if (!ctx->backlight)
            return -EPROBE_DEFER;
    }
#endif

    dev_set_drvdata(dev, ctx);

    ret = drm_panel_add(&ctx->panel);
    if (ret < 0)
    {
        dev_err(dev, "drm_panel_add failed\n");
        goto err_free_backlight;
    }
        
    mtk_dbi_host_attach(&ctx->panel, &lcm_param, &ops);

    return 0;

err_free_backlight:
    if (ctx->backlight)
        put_device(&ctx->backlight->dev);

    return ret;
}

static int jd9851_remove(struct device *dev)
{
    struct jd9851 *ctx = dev_get_drvdata(dev);

    drm_panel_remove(&ctx->panel);

    if (ctx->backlight)
        put_device(&ctx->backlight->dev);

    return 0;
}

static const struct of_device_id jd9851_of_match[] = {
    { .compatible = "mediatek,jd9851" },
    { }
};
MODULE_DEVICE_TABLE(of, jd9851_of_match);

static int jd9851_driver_platform_probe(struct platform_device *pdev)
{
    const struct of_device_id *id;
    id = of_match_node(jd9851_of_match, pdev->dev.of_node);
    if (!id) {
        return -ENODEV;
    }

    return jd9851_probe(&pdev->dev);
}

static int jd9851_driver_platform_remove(struct platform_device *pdev)
{
    return jd9851_remove(&pdev->dev);
}

/*
static void panel_simple_platform_shutdown(struct platform_device *pdev)
{
    panel_simple_shutdown(&pdev->dev);
}
*/

static struct platform_driver jd9851_driver_platform_driver = {
    .driver = {
        .name = "fitipower,jd9851",
        .of_match_table = jd9851_of_match,
    },
    .probe = jd9851_driver_platform_probe,
    .remove = jd9851_driver_platform_remove,
    //.shutdown = panel_simple_platform_shutdown,
};

static int __init jd9851_driver_init(void)
{
    int err;

    err = platform_driver_register(&jd9851_driver_platform_driver);
    if (err < 0) {
        return err;
    }

    return 0;
}
module_init(jd9851_driver_init);

static void __exit jd9851_driver_exit(void)
{
    platform_driver_unregister(&jd9851_driver_platform_driver);
}
module_exit(jd9851_driver_exit);

MODULE_DESCRIPTION("jd9851 LCD Driver");
MODULE_LICENSE("GPL v2");

