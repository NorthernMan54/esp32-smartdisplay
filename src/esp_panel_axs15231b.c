#ifdef DISPLAY_AXS15231B_QSPI

#include <esp_panel_axs15231b.h>
#include <esp32-hal-log.h>
#include <esp_rom_gpio.h>
#include <esp_heap_caps.h>
#include <memory.h>
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_interface.h>
#include <esp_lcd_panel_io.h>

typedef struct
{
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_dev_config_t config;
    // Data
    int x_gap;
    int y_gap;
    uint8_t madctl;
} axs15231b_panel_t;

const lcd_init_cmd_t axs15231b_vendor_specific_init_default[] = { // Borrowed from esp_bsp.c
    {0xBB, (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0xA5}, 8, 0},
    {0xA0, (uint8_t []){0xC0, 0x10, 0x00, 0x02, 0x00, 0x00, 0x04, 0x3F, 0x20, 0x05, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00}, 17, 0},
    {0xA2, (uint8_t []){0x30, 0x3C, 0x24, 0x14, 0xD0, 0x20, 0xFF, 0xE0, 0x40, 0x19, 0x80, 0x80, 0x80, 0x20, 0xf9, 0x10, 0x02, 0xff, 0xff, 0xF0, 0x90, 0x01, 0x32, 0xA0, 0x91, 0xE0, 0x20, 0x7F, 0xFF, 0x00, 0x5A}, 31, 0},
    {0xD0, (uint8_t []){0xE0, 0x40, 0x51, 0x24, 0x08, 0x05, 0x10, 0x01, 0x20, 0x15, 0x42, 0xC2, 0x22, 0x22, 0xAA, 0x03, 0x10, 0x12, 0x60, 0x14, 0x1E, 0x51, 0x15, 0x00, 0x8A, 0x20, 0x00, 0x03, 0x3A, 0x12}, 30, 0},
    {0xA3, (uint8_t []){0xA0, 0x06, 0xAa, 0x00, 0x08, 0x02, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x55, 0x55}, 22, 0},
    {0xC1, (uint8_t []){0x31, 0x04, 0x02, 0x02, 0x71, 0x05, 0x24, 0x55, 0x02, 0x00, 0x41, 0x00, 0x53, 0xFF, 0xFF, 0xFF, 0x4F, 0x52, 0x00, 0x4F, 0x52, 0x00, 0x45, 0x3B, 0x0B, 0x02, 0x0d, 0x00, 0xFF, 0x40}, 30, 0},
    {0xC3, (uint8_t []){0x00, 0x00, 0x00, 0x50, 0x03, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01}, 11, 0},
    {0xC4, (uint8_t []){0x00, 0x24, 0x33, 0x80, 0x00, 0xea, 0x64, 0x32, 0xC8, 0x64, 0xC8, 0x32, 0x90, 0x90, 0x11, 0x06, 0xDC, 0xFA, 0x00, 0x00, 0x80, 0xFE, 0x10, 0x10, 0x00, 0x0A, 0x0A, 0x44, 0x50}, 29, 0},
    {0xC5, (uint8_t []){0x18, 0x00, 0x00, 0x03, 0xFE, 0x3A, 0x4A, 0x20, 0x30, 0x10, 0x88, 0xDE, 0x0D, 0x08, 0x0F, 0x0F, 0x01, 0x3A, 0x4A, 0x20, 0x10, 0x10, 0x00}, 23, 0},
    {0xC6, (uint8_t []){0x05, 0x0A, 0x05, 0x0A, 0x00, 0xE0, 0x2E, 0x0B, 0x12, 0x22, 0x12, 0x22, 0x01, 0x03, 0x00, 0x3F, 0x6A, 0x18, 0xC8, 0x22}, 20, 0},
    {0xC7, (uint8_t []){0x50, 0x32, 0x28, 0x00, 0xa2, 0x80, 0x8f, 0x00, 0x80, 0xff, 0x07, 0x11, 0x9c, 0x67, 0xff, 0x24, 0x0c, 0x0d, 0x0e, 0x0f}, 20, 0},
    {0xC9, (uint8_t []){0x33, 0x44, 0x44, 0x01}, 4, 0},
    {0xCF, (uint8_t []){0x2C, 0x1E, 0x88, 0x58, 0x13, 0x18, 0x56, 0x18, 0x1E, 0x68, 0x88, 0x00, 0x65, 0x09, 0x22, 0xC4, 0x0C, 0x77, 0x22, 0x44, 0xAA, 0x55, 0x08, 0x08, 0x12, 0xA0, 0x08}, 27, 0},
    {0xD5, (uint8_t []){0x40, 0x8E, 0x8D, 0x01, 0x35, 0x04, 0x92, 0x74, 0x04, 0x92, 0x74, 0x04, 0x08, 0x6A, 0x04, 0x46, 0x03, 0x03, 0x03, 0x03, 0x82, 0x01, 0x03, 0x00, 0xE0, 0x51, 0xA1, 0x00, 0x00, 0x00}, 30, 0},
    {0xD6, (uint8_t []){0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE, 0x93, 0x00, 0x01, 0x83, 0x07, 0x07, 0x00, 0x07, 0x07, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x84, 0x00, 0x20, 0x01, 0x00}, 30, 0},
    {0xD7, (uint8_t []){0x03, 0x01, 0x0b, 0x09, 0x0f, 0x0d, 0x1E, 0x1F, 0x18, 0x1d, 0x1f, 0x19, 0x40, 0x8E, 0x04, 0x00, 0x20, 0xA0, 0x1F}, 19, 0},
    {0xD8, (uint8_t []){0x02, 0x00, 0x0a, 0x08, 0x0e, 0x0c, 0x1E, 0x1F, 0x18, 0x1d, 0x1f, 0x19}, 12, 0},
    {0xD9, (uint8_t []){0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, 12, 0},
    {0xDD, (uint8_t []){0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, 12, 0},
    {0xDF, (uint8_t []){0x44, 0x73, 0x4B, 0x69, 0x00, 0x0A, 0x02, 0x90}, 8,  0},
    {0xE0, (uint8_t []){0x3B, 0x28, 0x10, 0x16, 0x0c, 0x06, 0x11, 0x28, 0x5c, 0x21, 0x0D, 0x35, 0x13, 0x2C, 0x33, 0x28, 0x0D}, 17, 0},
    {0xE1, (uint8_t []){0x37, 0x28, 0x10, 0x16, 0x0b, 0x06, 0x11, 0x28, 0x5C, 0x21, 0x0D, 0x35, 0x14, 0x2C, 0x33, 0x28, 0x0F}, 17, 0},
    {0xE2, (uint8_t []){0x3B, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x35, 0x44, 0x32, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F, 0x0D}, 17, 0},
    {0xE3, (uint8_t []){0x37, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x35, 0x44, 0x32, 0x0C, 0x14, 0x14, 0x36, 0x32, 0x2F, 0x0F}, 17, 0},
    {0xE4, (uint8_t []){0x3B, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x39, 0x44, 0x2E, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F, 0x0D}, 17, 0},
    {0xE5, (uint8_t []){0x37, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x39, 0x44, 0x2E, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F, 0x0F}, 17, 0},
    {0xA4, (uint8_t []){0x85, 0x85, 0x95, 0x82, 0xAF, 0xAA, 0xAA, 0x80, 0x10, 0x30, 0x40, 0x40, 0x20, 0xFF, 0x60, 0x30}, 16, 0},
    {0xA4, (uint8_t []){0x85, 0x85, 0x95, 0x85}, 4, 0},
    {0xBB, (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 8, 0},
    {0x13, (uint8_t []){0x00}, 0, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x2C, (uint8_t []){0x00, 0x00, 0x00, 0x00}, 4, 0},
};

esp_err_t axs15231b_reset(esp_lcd_panel_t *panel)
{
    log_v("panel:0x%08x", panel);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    const axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    if (ph->config.reset_gpio_num != GPIO_NUM_NC)
    {
        // Hardware reset
        gpio_set_level(ph->config.reset_gpio_num, ph->config.flags.reset_active_high);
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_set_level(ph->config.reset_gpio_num, !ph->config.flags.reset_active_high);
    }
    else
    {
        esp_err_t res;
        // Software reset
        if ((res = esp_lcd_panel_io_tx_param(ph->io, LCD_CMD_SWRESET, NULL, 0)) != ESP_OK)
        {
            log_e("Sending LCD_CMD_SWRESET failed");
            return res;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(5));

    return ESP_OK;
}

esp_err_t axs15231b_init(esp_lcd_panel_t *panel)
{
    log_v("panel:0x%08x", panel);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    const axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    esp_err_t res;
    if ((res = esp_lcd_panel_io_tx_param(ph->io, LCD_CMD_SLPOUT, NULL, 0)) != ESP_OK)
    {
        log_e("Sending SLPOUT failed");
        return res;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t colmod;
    switch (ph->config.bits_per_pixel)
    {
    case 16: // RGB565
        colmod = 0x05;
        break;
    case 18: // RGB666
        colmod = 0x06;
        break;
    case 24: // RGB888
        colmod = 0x07;
        break;
    default:
        log_e("Invalid bits per pixel: %d. Only RGB565, RGB666 and RGB888 are supported", ph->config.bits_per_pixel);
        return ESP_ERR_INVALID_ARG;
    }

    if ((res = esp_lcd_panel_io_tx_param(ph->io, LCD_CMD_MADCTL, &ph->madctl, 1)) != ESP_OK ||
        (res = esp_lcd_panel_io_tx_param(ph->io, LCD_CMD_COLMOD, &colmod, 1)) != ESP_OK)
    {
        log_e("Sending MADCTL/COLMOD failed");
        return res;
    }

    const lcd_init_cmd_t *cmd = axs15231b_vendor_specific_init_default;
    uint16_t cmds_size = sizeof(axs15231b_vendor_specific_init_default) / sizeof(lcd_init_cmd_t);
    if (ph->config.vendor_config != NULL)
    {
        cmd = ((axs15231b_vendor_config_t *)ph->config.vendor_config)->init_cmds;
        cmds_size = ((axs15231b_vendor_config_t *)ph->config.vendor_config)->init_cmds_size;
    }

    while (cmds_size-- > 0)
    {
        if ((res = esp_lcd_panel_io_tx_param(ph->io, cmd->cmd, cmd->data, cmd->bytes)) != ESP_OK)
        {
            log_e("Sending command: 0x%02x failed", cmd->cmd);
            return res;
        }

        vTaskDelay(pdMS_TO_TICKS(cmd->delay_ms));
        cmd++;
    }

    return ESP_OK;
}

esp_err_t axs15231b_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    log_v("panel:0x%08x, x_start:%d, y_start:%d, x_end:%d, y_end:%d, color_data:0x%08x", panel, x_start, y_start, x_end, y_end, color_data);
    if (panel == NULL || color_data == NULL)
        return ESP_ERR_INVALID_ARG;

    const axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    if (x_start >= x_end)
    {
        log_w("X-start greater than the x-end");
        return ESP_ERR_INVALID_ARG;
    }

    if (y_start >= y_end)
    {
        log_w("Y-start greater than the y-end");
        return ESP_ERR_INVALID_ARG;
    }

    // Correct for gap
    x_start += ph->x_gap;
    x_end += ph->x_gap;
    y_start += ph->y_gap;
    y_end += ph->y_gap;

    esp_err_t res;
    const uint8_t caset[4] = {x_start >> 8, x_start, (x_end - 1) >> 8, (x_end - 1)};
    const uint8_t raset[4] = {y_start >> 8, y_start, (y_end - 1) >> 8, (y_end - 1)};
    if ((res = esp_lcd_panel_io_tx_param(ph->io, LCD_CMD_CASET, caset, sizeof(caset))) != ESP_OK ||
        (res = esp_lcd_panel_io_tx_param(ph->io, LCD_CMD_RASET, raset, sizeof(raset))) != ESP_OK)
    {
        log_e("Sending CASET/RASET failed");
        return res;
    }

    uint8_t bytes_per_pixel = (ph->config.bits_per_pixel + 0x7) >> 3;
    size_t len = (x_end - x_start) * (y_end - y_start) * bytes_per_pixel;
    if ((res = esp_lcd_panel_io_tx_color(ph->io, LCD_CMD_RAMWR, color_data, len)) != ESP_OK)
    {
        log_e("Sending RAMWR failed");
        return res;
    }

    return ESP_OK;
}

esp_err_t axs15231b_invert_color(esp_lcd_panel_t *panel, bool invert)
{
    log_v("panel:0x%08x, invert:%d", panel, invert);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    const axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    esp_err_t res;
    if ((res = esp_lcd_panel_io_tx_param(ph->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0)) != ESP_OK)
    {
        log_e("Sending LCD_CMD_INVON/LCD_CMD_INVOFF failed");
        return res;
    }

    return ESP_OK;
}

esp_err_t axs15231b_update_madctl(axs15231b_panel_t *ph)
{
    esp_err_t res;
    if ((res = esp_lcd_panel_io_tx_param(ph->io, LCD_CMD_MADCTL, &ph->madctl, 1)) != ESP_OK)
    {
        log_e("Sending LCD_CMD_MADCTL failed");
        return res;
    }

    return ESP_OK;
}

esp_err_t axs15231b_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    log_v("panel:0x%08x, mirror_x:%d, mirror_y:%d", panel, mirror_x, mirror_y);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    if (mirror_x)
        ph->madctl |= LCD_CMD_MX_BIT;
    else
        ph->madctl &= ~LCD_CMD_MX_BIT;

    if (mirror_y)
        ph->madctl |= LCD_CMD_MY_BIT;
    else
        ph->madctl &= ~LCD_CMD_MY_BIT;

    return axs15231b_update_madctl(ph);
}

esp_err_t axs15231b_swap_xy(esp_lcd_panel_t *panel, bool swap_xy)
{
    log_v("panel:0x%08x, swap_xy:%d", panel, swap_xy);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    if (swap_xy)
        ph->madctl |= LCD_CMD_MV_BIT;
    else
        ph->madctl &= ~LCD_CMD_MV_BIT;

    return axs15231b_update_madctl(ph);
}

esp_err_t axs15231b_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    log_v("panel:0x%08x, x_gap:%d, y_gap:%d", panel, x_gap, y_gap);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    ph->x_gap = x_gap;
    ph->y_gap = y_gap;

    return ESP_OK;
}

esp_err_t axs15231b_disp_off(esp_lcd_panel_t *panel, bool off)
{
    log_v("panel:0x%08x, off:%d", panel, off);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    const axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    esp_err_t res;
    if ((res = esp_lcd_panel_io_tx_param(ph->io, off ? LCD_CMD_DISPOFF : LCD_CMD_DISPON, NULL, 0)) != ESP_OK)
    {
        log_e("Sending LCD_CMD_DISPOFF/LCD_CMD_DISPON failed");
        return res;
    }

    return ESP_OK;
}

esp_err_t axs15231b_del(esp_lcd_panel_t *panel)
{
    log_v("panel:0x%08x", panel);
    if (panel == NULL)
        return ESP_ERR_INVALID_ARG;

    axs15231b_panel_t *ph = (axs15231b_panel_t *)panel;

    // Reset RESET
    if (ph->config.reset_gpio_num != GPIO_NUM_NC)
        gpio_reset_pin(ph->config.reset_gpio_num);

    free(ph);

    return ESP_OK;
}

esp_err_t esp_lcd_new_panel_axs15231b(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *config, esp_lcd_panel_handle_t *handle)
{
    log_v("io:0x%08x, config:0x%08x, handle:0x%08x", io, config, handle);
    if (io == NULL || config == NULL || handle == NULL)
        return ESP_ERR_INVALID_ARG;

    if (config->reset_gpio_num != GPIO_NUM_NC && !GPIO_IS_VALID_GPIO(config->reset_gpio_num))
    {
        log_e("Invalid GPIO RST pin: %d", config->reset_gpio_num);
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t madctl;
    switch (config->color_space)
    {
    case ESP_LCD_COLOR_SPACE_RGB:
        madctl = 0;
        break;
    case ESP_LCD_COLOR_SPACE_BGR:
        madctl = LCD_CMD_BGR_BIT;
        break;
    default:
        log_e("Invalid color space: %d. Only RGB and BGR are supported", config->color_space);
        return ESP_ERR_INVALID_ARG;
    }

    if (config->reset_gpio_num != GPIO_NUM_NC)
    {
        esp_err_t res;
        const gpio_config_t cfg = {
            .pin_bit_mask = BIT64(config->reset_gpio_num),
            .mode = GPIO_MODE_OUTPUT};
        if ((res = gpio_config(&cfg)) != ESP_OK)
        {
            log_e("Configuring GPIO for RST failed");
            return res;
        }
    }

    axs15231b_panel_t *ph = heap_caps_calloc(1, sizeof(axs15231b_panel_t), MALLOC_CAP_DEFAULT);
    if (ph == NULL)
    {
        log_e("No memory available for axs15231b_panel_t");
        return ESP_ERR_NO_MEM;
    }

    ph->io = io;
    memcpy(&ph->config, config, sizeof(esp_lcd_panel_dev_config_t));
    ph->madctl = madctl;

    ph->base.del = axs15231b_del;
    ph->base.reset = axs15231b_reset;
    ph->base.init = axs15231b_init;
    ph->base.draw_bitmap = axs15231b_draw_bitmap;
    ph->base.invert_color = axs15231b_invert_color;
    ph->base.mirror = axs15231b_mirror;
    ph->base.swap_xy = axs15231b_swap_xy;
    ph->base.set_gap = axs15231b_set_gap;
    ph->base.disp_off = axs15231b_disp_off;

    log_d("handle: 0x%08x", ph);
    *handle = (esp_lcd_panel_handle_t)ph;

    return ESP_OK;
}

#endif