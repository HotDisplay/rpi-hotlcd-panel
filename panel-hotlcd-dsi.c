/*
 * Copyright (C) 2026 HOTDISPLAY Technology Co., Ltd
 *
 * Based on panel-raspberrypi-touchscreen by Broadcom
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

struct ht_panel_desc {
    const struct panel_init_cmd *init;
    const struct drm_display_mode *mode;
    const unsigned long mode_flags;
    unsigned int lanes;
    enum mipi_dsi_pixel_format format;
};

struct ht_panel {
    struct drm_panel panel;
    struct mipi_dsi_device *dsi;
    const struct ht_panel_desc *desc;

    struct regulator *power;
    struct gpio_desc *reset;
    struct gpio_desc *iovcc;
    struct gpio_desc *avdd;

    enum drm_panel_orientation orientation;
};

enum dsi_cmd_type {
    INIT_DCS_CMD,
    DELAY_CMD,
};

struct panel_init_cmd {
    enum dsi_cmd_type type;
    size_t len;
    const char *data;
};

#define INIT_DCS_SEND(...)                                                    \
	{                                                                     \
		.type = INIT_DCS_CMD, .len = sizeof((char[]){ __VA_ARGS__ }), \
		.data = (char[])                                              \
		{                                                             \
			__VA_ARGS__                                           \
		}                                                             \
	}

#define DELAY_MS(...)                                               \
	{                                                                  \
		.type = DELAY_CMD, .len = sizeof((char[]){ __VA_ARGS__ }), \
		.data = (char[])                                           \
		{                                                          \
			__VA_ARGS__                                        \
		}                                                          \
	}




static const struct panel_init_cmd ht_panel_h070a23_init[] = {
    INIT_DCS_SEND(0xee, 0x01),
    INIT_DCS_SEND(0xea, 0x07),
    INIT_DCS_SEND(0xeb, 0x12),
    INIT_DCS_SEND(0x05, 0x12),
    INIT_DCS_SEND(0x0a, 0x87),
    INIT_DCS_SEND(0x0c, 0x20),
    INIT_DCS_SEND(0x13, 0x14),
    INIT_DCS_SEND(0x15, 0x58),
    INIT_DCS_SEND(0x17, 0x32),
    INIT_DCS_SEND(0x1d, 0x33),
    INIT_DCS_SEND(0x21, 0x01),
    INIT_DCS_SEND(0x28, 0x23),
    INIT_DCS_SEND(0x29, 0x23),
    INIT_DCS_SEND(0x2a, 0x03),
    INIT_DCS_SEND(0x2f, 0xf3),
    INIT_DCS_SEND(0xee, 0x02),
    INIT_DCS_SEND(0x39, 0xB0),

    INIT_DCS_SEND(0x00, 0x00),
    INIT_DCS_SEND(0x01, 0x11),
    INIT_DCS_SEND(0x02, 0x18),
    INIT_DCS_SEND(0x03, 0x0D),
    INIT_DCS_SEND(0x04, 0x15),
    INIT_DCS_SEND(0x05, 0x36),
    INIT_DCS_SEND(0x06, 0x0E),
    INIT_DCS_SEND(0x07, 0x10),
    INIT_DCS_SEND(0x08, 0x11),
    INIT_DCS_SEND(0x09, 0x0F),
    INIT_DCS_SEND(0x0A, 0x12),
    INIT_DCS_SEND(0x0b, 0x5a),
    INIT_DCS_SEND(0x0c, 0x13),
    INIT_DCS_SEND(0x0d, 0x18),
    INIT_DCS_SEND(0x0e, 0x3A),
    INIT_DCS_SEND(0x0f, 0x3D),
    INIT_DCS_SEND(0x10, 0x3F),

    INIT_DCS_SEND(0x20, 0x00),
    INIT_DCS_SEND(0x21, 0x11),
    INIT_DCS_SEND(0x22, 0x18),
    INIT_DCS_SEND(0x23, 0x0D),
    INIT_DCS_SEND(0x24, 0x15),
    INIT_DCS_SEND(0x25, 0x36),
    INIT_DCS_SEND(0x26, 0x0E),
    INIT_DCS_SEND(0x27, 0x10),
    INIT_DCS_SEND(0x28, 0x11),
    INIT_DCS_SEND(0x29, 0x0F),
    INIT_DCS_SEND(0x2A, 0x12),
    INIT_DCS_SEND(0x2b, 0x5a),
    INIT_DCS_SEND(0x2c, 0x13),
    INIT_DCS_SEND(0x2d, 0x18),
    INIT_DCS_SEND(0x2e, 0x3A),
    INIT_DCS_SEND(0x2f, 0x3D),
    INIT_DCS_SEND(0x30, 0x3F),

    INIT_DCS_SEND(0xee, 0x03),
    INIT_DCS_SEND(0x0f, 0xB9),
    INIT_DCS_SEND(0x10, 0x00),
    INIT_DCS_SEND(0xee, 0x04),
    INIT_DCS_SEND(0x00, 0x05),
    INIT_DCS_SEND(0x01, 0x01),
    INIT_DCS_SEND(0x02, 0x2C),
    INIT_DCS_SEND(0x03, 0x04),
    INIT_DCS_SEND(0x04, 0x00),
    INIT_DCS_SEND(0x06, 0x06),
    INIT_DCS_SEND(0x07, 0x05),
    INIT_DCS_SEND(0x08, 0x15),
    INIT_DCS_SEND(0x09, 0x20),
    INIT_DCS_SEND(0x0a, 0x0a),
    INIT_DCS_SEND(0x0b, 0x00),
    INIT_DCS_SEND(0x0f, 0x0a),
    INIT_DCS_SEND(0x19, 0xcc),
    INIT_DCS_SEND(0x1a, 0xcc),
    INIT_DCS_SEND(0x20, 0x40),
    INIT_DCS_SEND(0x23, 0x38),
    INIT_DCS_SEND(0x24, 0x08),
    INIT_DCS_SEND(0x25, 0x02),
    INIT_DCS_SEND(0x29, 0x00),
    INIT_DCS_SEND(0x30, 0x1d),
    INIT_DCS_SEND(0x31, 0x1d),
    INIT_DCS_SEND(0x37, 0x22),
    INIT_DCS_SEND(0x40, 0x80),
    INIT_DCS_SEND(0x41, 0x55),

    INIT_DCS_SEND(0xee, 0x05),
    INIT_DCS_SEND(0x00, 0x01),
    INIT_DCS_SEND(0x01, 0x05),
    INIT_DCS_SEND(0x02, 0x45),
    INIT_DCS_SEND(0x03, 0x05),
    INIT_DCS_SEND(0x07, 0xbD),
    INIT_DCS_SEND(0x08, 0xc1),
    INIT_DCS_SEND(0x09, 0x44),
    INIT_DCS_SEND(0x10, 0x03),
    INIT_DCS_SEND(0x11, 0x07),
    INIT_DCS_SEND(0x12, 0x45),
    INIT_DCS_SEND(0x13, 0x05),
    INIT_DCS_SEND(0x19, 0xBB),
    INIT_DCS_SEND(0x1a, 0x74),
    INIT_DCS_SEND(0x30, 0x01),
    INIT_DCS_SEND(0x31, 0x01),
    INIT_DCS_SEND(0x32, 0x00),
    INIT_DCS_SEND(0x33, 0x14),
    INIT_DCS_SEND(0x34, 0x14),
    INIT_DCS_SEND(0x35, 0x78),
    INIT_DCS_SEND(0x36, 0x01),
    INIT_DCS_SEND(0x37, 0x01),
    INIT_DCS_SEND(0x38, 0x00),
    INIT_DCS_SEND(0x39, 0x14),
    INIT_DCS_SEND(0x3A, 0x14),
    INIT_DCS_SEND(0x40, 0xEE),
    INIT_DCS_SEND(0x41, 0x44),
    INIT_DCS_SEND(0x43, 0x13),
    INIT_DCS_SEND(0x44, 0x01),
    INIT_DCS_SEND(0x45, 0x81),
    INIT_DCS_SEND(0x46, 0x06),
    INIT_DCS_SEND(0x47, 0x00),

    INIT_DCS_SEND(0xee, 0x06),
    INIT_DCS_SEND(0x00, 0x01),
    INIT_DCS_SEND(0x02, 0x45),
    INIT_DCS_SEND(0x06, 0xcd),
    INIT_DCS_SEND(0x08, 0x67),
    INIT_DCS_SEND(0x09, 0x45),
    INIT_DCS_SEND(0x0a, 0x23),
    INIT_DCS_SEND(0x0b, 0x01),
    INIT_DCS_SEND(0xee, 0x07),
    INIT_DCS_SEND(0x00, 0x01),
    INIT_DCS_SEND(0x01, 0x05),
    INIT_DCS_SEND(0x02, 0x0C),
    INIT_DCS_SEND(0x03, 0x0D),
    INIT_DCS_SEND(0x04, 0x3c),
    INIT_DCS_SEND(0x05, 0x21),
    INIT_DCS_SEND(0x06, 0x20),
    INIT_DCS_SEND(0x07, 0x12),
    INIT_DCS_SEND(0x08, 0x10),
    INIT_DCS_SEND(0x09, 0x16),
    INIT_DCS_SEND(0x0A, 0x14),
    INIT_DCS_SEND(0x0b, 0x3C),
    INIT_DCS_SEND(0x0c, 0x3C),
    INIT_DCS_SEND(0x0d, 0x3C),
    INIT_DCS_SEND(0x0e, 0x3C),
    INIT_DCS_SEND(0x0f, 0x3C),
    INIT_DCS_SEND(0x10, 0x3c),
    INIT_DCS_SEND(0x11, 0x3c),
    INIT_DCS_SEND(0x12, 0x3c),
    INIT_DCS_SEND(0x13, 0x3c),
    INIT_DCS_SEND(0x14, 0x3c),
    INIT_DCS_SEND(0x15, 0x3c),
    INIT_DCS_SEND(0x20, 0x00),
    INIT_DCS_SEND(0x21, 0x04),
    INIT_DCS_SEND(0x22, 0x0C),
    INIT_DCS_SEND(0x23, 0x0D),
    INIT_DCS_SEND(0x24, 0x3c),
    INIT_DCS_SEND(0x25, 0x21),
    INIT_DCS_SEND(0x26, 0x20),
    INIT_DCS_SEND(0x27, 0x13),
    INIT_DCS_SEND(0x28, 0x11),
    INIT_DCS_SEND(0x29, 0x17),
    INIT_DCS_SEND(0x2A, 0x15),
    INIT_DCS_SEND(0x2b, 0x3C),
    INIT_DCS_SEND(0x2c, 0x3C),
    INIT_DCS_SEND(0x2d, 0x3C),
    INIT_DCS_SEND(0x2e, 0x3C),
    INIT_DCS_SEND(0x2f, 0x3C),
    INIT_DCS_SEND(0x30, 0x3c),
    INIT_DCS_SEND(0x31, 0x3c),
    INIT_DCS_SEND(0x32, 0x3c),
    INIT_DCS_SEND(0x33, 0x3c),
    INIT_DCS_SEND(0x34, 0x3c),
    INIT_DCS_SEND(0x35, 0x3c),

    INIT_DCS_SEND(0xee, 0x08),
    INIT_DCS_SEND(0x10, 0x00),	// 00=4Lane，01=3Lane，02=2LANE。
    INIT_DCS_SEND(0x12, 0xda),
    INIT_DCS_SEND(0x13, 0x1c),
    INIT_DCS_SEND(0x18, 0x30),
    INIT_DCS_SEND(0x41, 0x80),
    INIT_DCS_SEND(0xee, 0x0f),
    INIT_DCS_SEND(0x00, 0x01),

    INIT_DCS_SEND(0xee, 0x00),
    INIT_DCS_SEND(0xea, 0x00),
    INIT_DCS_SEND(0xeb, 0x00),
    INIT_DCS_SEND(0x36, 0x00),
    {},
};



static inline struct ht_panel *panel_to_ht(struct drm_panel *panel) {
    return container_of(panel, struct ht_panel, panel);
}

static int ht_panel_init_dcs_cmd(struct ht_panel *ts) {
    struct mipi_dsi_device *dsi = ts->dsi;
    struct drm_panel *panel = &ts->panel;
    int i, err = 0;

    if (ts->desc->init) {
        const struct panel_init_cmd *init_cmds = ts->desc->init;

        for (i = 0; init_cmds[i].len != 0; i++) {
            const struct panel_init_cmd *cmd = &init_cmds[i];

            switch (cmd->type) {
            case DELAY_CMD:
                msleep(cmd->data[0]);
                err = 0;
                break;

            case INIT_DCS_CMD:
                err = mipi_dsi_dcs_write(
                    dsi, cmd->data[0],
                    cmd->len <= 1 ? NULL : &cmd->data[1],
                    cmd->len - 1);
                break;

            default:
                err = -EINVAL;
            }

            if (err < 0) {
                dev_err(panel->dev,
                    "failed to write command %u\n", i);
                return err;
            }
        }
    }
    return 0;
}

static int ht_panel_prepare(struct drm_panel *panel) {
    struct ht_panel *ctx = panel_to_ht(panel);
    int ret;

    if (ctx->power) {
        ret = regulator_enable(ctx->power);
        if (ret)
            return ret;
    }

    /* And reset it */
    if (ctx->reset) {
        gpiod_set_value_cansleep(ctx->reset, 1);
        msleep(50);
        gpiod_set_value_cansleep(ctx->reset, 0);
        msleep(50);
    }

    ret = ht_panel_init_dcs_cmd(ctx);
    if (ret < 0) {
        dev_err(panel->dev, "failed to init panel: %d\n", ret);
        if (ctx->power)
            regulator_disable(ctx->power);
    }

    return ret;
}

static int ht_panel_unprepare(struct drm_panel *panel) {
    struct ht_panel *ctx = panel_to_ht(panel);

    mipi_dsi_dcs_set_display_off(ctx->dsi);
    mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);

    if (ctx->reset) {
        gpiod_set_value_cansleep(ctx->reset, 1);
        msleep(20);
    }

    if (ctx->power)
        return regulator_disable(ctx->power);

    return 0;
}



static const struct drm_display_mode ht_panel_h070a23_mode = {
    .clock = 51 * 1000,
    .hdisplay = 1024,
    .hsync_start = 1024 + 120,
    .hsync_end = 1024 + 120 + 80,
    .htotal = 1024 + 120 + 80 + 80,
    .vdisplay = 600,
    .vsync_start = 600 + 16,
    .vsync_end = 600 + 16 + 8,
    .vtotal = 600 + 16 + 8 + 8,
    .width_mm = 165,
    .height_mm = 100,
};



static int ht_panel_get_modes(struct drm_panel *panel, struct drm_connector *connector) {
    struct ht_panel *ctx = panel_to_ht(panel);
    struct drm_display_mode *mode;

    mode = drm_mode_duplicate(connector->dev, ctx->desc->mode);
    if (!mode) {
        dev_err(&ctx->dsi->dev, "failed to add mode %ux%ux@%u\n",
            ctx->desc->mode->hdisplay, ctx->desc->mode->vdisplay,
            drm_mode_vrefresh(ctx->desc->mode));
        return -ENOMEM;
    }

    drm_mode_set_name(mode);

    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode);

    connector->display_info.width_mm = mode->width_mm;
    connector->display_info.height_mm = mode->height_mm;

    drm_connector_set_panel_orientation(connector, ctx->orientation);

    return 1;
}

static enum drm_panel_orientation ht_panel_get_orientation(struct drm_panel *panel) {
    struct ht_panel *ctx = panel_to_ht(panel);

    return ctx->orientation;
}

static const struct drm_panel_funcs ht_panel_funcs = {
    .prepare = ht_panel_prepare,
    .unprepare = ht_panel_unprepare,
    .get_modes = ht_panel_get_modes,
    .get_orientation = ht_panel_get_orientation,
};

static int ht_panel_dsi_probe(struct mipi_dsi_device *dsi) {
    struct ht_panel *ctx;
    int ret;

    dev_info(&dsi->dev, "dsi panel: %s\n", (char *)of_get_property(dsi->dev.of_node, "compatible", NULL));

    ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;
    mipi_dsi_set_drvdata(dsi, ctx);
    ctx->dsi = dsi;
    ctx->desc = of_device_get_match_data(&dsi->dev);

    ctx->panel.prepare_prev_first = true;
    drm_panel_init(&ctx->panel, &dsi->dev, &ht_panel_funcs,
        DRM_MODE_CONNECTOR_DSI);

    ctx->power = devm_regulator_get_optional(&dsi->dev, "power");
    if (IS_ERR(ctx->power)) {
        if (PTR_ERR(ctx->power) != -ENODEV)
            return dev_err_probe(&dsi->dev, PTR_ERR(ctx->power), "Couldn't get power supply\n");
        ctx->power = NULL;
    }

    ctx->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->reset))
        return dev_err_probe(&dsi->dev, PTR_ERR(ctx->reset), "Couldn't get reset GPIO\n");


    ret = of_drm_get_panel_orientation(dsi->dev.of_node, &ctx->orientation);
    if (ret) {
        dev_err(&dsi->dev, "%pOF: failed to get orientation: %d\n",
            dsi->dev.of_node, ret);
        return ret;
    }

    ret = drm_panel_of_backlight(&ctx->panel);
    if (ret)
        return ret;

    drm_panel_add(&ctx->panel);

    dsi->mode_flags = ctx->desc->mode_flags;
    dsi->format = ctx->desc->format;
    dsi->lanes = ctx->desc->lanes;
    dev_info(&dsi->dev, "lanes: %d\n", dsi->lanes);

    ret = mipi_dsi_attach(dsi);
    if (ret)
        drm_panel_remove(&ctx->panel);

    return ret;
}

static void ht_panel_dsi_remove(struct mipi_dsi_device *dsi) {
    struct ht_panel *ctx = mipi_dsi_get_drvdata(dsi);

    mipi_dsi_detach(dsi);
    drm_panel_remove(&ctx->panel);
    if (ctx->reset) {
        gpiod_set_value_cansleep(ctx->reset, 0);
        msleep(20);
    }

}

static void ht_panel_dsi_shutdown(struct mipi_dsi_device *dsi) {
    struct ht_panel *ctx = mipi_dsi_get_drvdata(dsi);

    if (ctx->reset) {
        dev_info(&dsi->dev, "shutdown\n");
        gpiod_set_value_cansleep(ctx->reset, 0);
        msleep(20);
    }

}



static const struct ht_panel_desc ht_panel_h070a23_desc = {
    .init = ht_panel_h070a23_init,
    .mode = &ht_panel_h070a23_mode,
    .mode_flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS,
    .lanes = 4,
    .format = MIPI_DSI_FMT_RGB888,
};


static const struct of_device_id ht_panel_of_match[] = {
    {.compatible = "hotlcd,h070a23", &ht_panel_h070a23_desc },
    {}
};
MODULE_DEVICE_TABLE(of, ht_panel_of_match);

static struct mipi_dsi_driver ht_panel_dsi_driver = {
    .probe = ht_panel_dsi_probe,
    .remove = ht_panel_dsi_remove,
    .shutdown = ht_panel_dsi_shutdown,
    .driver = {
        .name = "hotlcd-dsi",
        .of_match_table = ht_panel_of_match,
    },
};
module_mipi_dsi_driver(ht_panel_dsi_driver);

MODULE_AUTHOR("HOTLCD <lcd@hotlcd.com");
MODULE_DESCRIPTION("HOTLCD H070A23 CM4 DSI panel driver");
MODULE_LICENSE("GPL");

