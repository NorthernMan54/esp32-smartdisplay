#ifdef DISPLAY_AXS15231B_QSPI

#include <esp32_smartdisplay.h>
#include <esp_panel_axs15231b.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>

#define MEMORY_DEBUG log_i( "\n  Free Heap:           %7zu bytes\n" "  MALLOC_CAP_8BIT      %7zu bytes\n" "  MALLOC_CAP_DMA       %7zu bytes\n" "  MALLOC_CAP_SPIRAM    %7zu bytes\n" "  MALLOC_CAP_INTERNAL  %7zu bytes\n" "  MALLOC_CAP_DEFAULT   %7zu bytes\n" "  MALLOC_CAP_IRAM_8BIT %7zu bytes\n" "  MALLOC_CAP_RETENTION %7zu bytes\n", xPortGetFreeHeapSize(), heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_free_size(MALLOC_CAP_DMA), heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size(MALLOC_CAP_DEFAULT), heap_caps_get_free_size(MALLOC_CAP_IRAM_8BIT), heap_caps_get_free_size(MALLOC_CAP_RETENTION))

bool axs15231b_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *display = user_ctx;
    lv_display_flush_ready(display);
    return false;
}

void axs15231b_lv_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = display->user_data;
    uint32_t pixels = lv_area_get_size(area);
    uint16_t *p = (uint16_t *)px_map;
    while (pixels--)
    {
        *p = (uint16_t)((*p >> 8) | (*p << 8));
        p++;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map));
};

lv_display_t *lvgl_lcd_init(uint32_t hor_res, uint32_t ver_res)
{
    lv_display_t *display = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    log_v("display:0x%08x", display);
    //  Create drawBuffer
    uint32_t drawBufferSize = sizeof(lv_color_t) * LVGL_BUFFER_PIXELS;
    log_v("display:%i", LVGL_BUFFER_PIXELS);
    MEMORY_DEBUG;
    void *drawBuffer = heap_caps_malloc(drawBufferSize, LVGL_BUFFER_MALLOC_FLAGS);
    log_v("display:0x%08x", display);
    lv_display_set_buffers(display, drawBuffer, NULL, drawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Hardware rotation is supported
    display->sw_rotate = 0;
    display->rotation = LV_DISPLAY_ROTATION_0;

    // Create SPI bus
    // From esp_bsp.c { .sclk_io_num = (GPIO_NUM_47), .data0_io_num = (GPIO_NUM_21), .data1_io_num = (GPIO_NUM_48), .data2_io_num = (GPIO_NUM_40), .data3_io_num = (GPIO_NUM_39), .max_transfer_sz = config->max_transfer_sz, }

    // [  5420][D][lvgl_panel_axs15231b_qspi.c:55] lvgl_lcd_init(): spi_bus_config: mosi_io_num:21, miso_io_num:48, sclk_io_num:47, quadwp_io_num:40, quadhd_io_num:39, max_transfer_sz:76800, flags:0x00000000, intr_flags:0x0000

    log_d("configure spi_bus_initialize: AXS15231B_Q");

    // [  5180][I][Arduino_ESP32QSPI.cpp:61] begin(): spi_bus_config: mosi_io_num:21, miso_io_num:48, sclk_io_num:47, quadwp_io_num:40, quadhd_io_num:39, max_transfer_sz:16392, flags:0x00000005, intr_flags:0x0000

    //                                                spi_bus_config: mosi_io_num:21, miso_io_num:48, sclk_io_num:47, quadwp_io_num:40, quadhd_io_num:39, max_transfer_sz:16388, flags:0x00000005, intr_flags:0x0000

    const spi_bus_config_t spi_bus_config = {
        .mosi_io_num = AXS15231B_QSPI_BUS_MOSI_IO_NUM,
        .miso_io_num = AXS15231B_QSPI_BUS_MISO_IO_NUM,
        .sclk_io_num = AXS15231B_QSPI_BUS_SCLK_IO_NUM,
        .quadwp_io_num = AXS15231B_QSPI_BUS_QUADWP_IO_NUM,
        .quadhd_io_num = AXS15231B_QSPI_BUS_QUADHD_IO_NUM,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = AXS15231B_QSPI_BUS_MAX_TRANSFER_SZ,
        .flags = AXS15231B_QSPI_BUS_FLAGS,
        .intr_flags = AXS15231B_QSPI_BUS_INTR_FLAGS};

    log_d("spi_bus_config: mosi_io_num:%d, miso_io_num:%d, sclk_io_num:%d, quadwp_io_num:%d, quadhd_io_num:%d, max_transfer_sz:%d, flags:0x%08x, intr_flags:0x%04x", spi_bus_config.mosi_io_num, spi_bus_config.miso_io_num, spi_bus_config.sclk_io_num, spi_bus_config.quadwp_io_num, spi_bus_config.quadhd_io_num, spi_bus_config.max_transfer_sz, spi_bus_config.flags, spi_bus_config.intr_flags);

    ESP_ERROR_CHECK_WITHOUT_ABORT(spi_bus_initialize(AXS15231B_QSPI_HOST, &spi_bus_config, AXS15231B_QSPI_DMA_CHANNEL));
    log_d("spi_bus_initialize: AXS15231B_Q");
    // Attach the LCD controller to the SPI bus
    // From esp_bsp.c { .cs_gpio_num = (GPIO_NUM_45), .dc_gpio_num = -1, .spi_mode = 3, .pclk_hz = 40 * 1000 * 1000, .trans_queue_depth = 10, .on_color_trans_done = ((void *)0), .user_ctx = ((void *)0), .lcd_cmd_bits = 32, .lcd_param_bits = 8, .flags = { .quad_mode = 1, }, }

    const esp_lcd_panel_io_spi_config_t io_spi_config = {
        .cs_gpio_num = AXS15231B_QSPI_CONFIG_CS_GPIO_NUM, // Verfified
        .dc_gpio_num = AXS15231B_QSPI_CONFIG_DC_GPIO_NUM, //
        .spi_mode = AXS15231B_QSPI_CONFIG_SPI_MODE,
        .pclk_hz = AXS15231B_QSPI_CONFIG_PCLK_HZ,
        .on_color_trans_done = ((void *)0),
        .user_ctx = ((void *)0),
        .trans_queue_depth = AXS15231B_QSPI_CONFIG_TRANS_QUEUE_DEPTH,
        .lcd_cmd_bits = AXS15231B_QSPI_CONFIG_LCD_CMD_BITS,
        .lcd_param_bits = AXS15231B_QSPI_CONFIG_LCD_PARAM_BITS,
        .flags = {
            .dc_as_cmd_phase = AXS15231B_QSPI_CONFIG_FLAGS_DC_AS_CMD_PHASE,
            .dc_low_on_data = AXS15231B_QSPI_CONFIG_FLAGS_DC_LOW_ON_DATA,
            .octal_mode = AXS15231B_QSPI_CONFIG_FLAGS_OCTAL_MODE,
            .lsb_first = AXS15231B_QSPI_CONFIG_FLAGS_LSB_FIRST}};

    log_d("io_spi_config: cs_gpio_num:%d, dc_gpio_num:%d, spi_mode:%d, pclk_hz:%d, trans_queue_depth:%d, user_ctx:0x%08x, on_color_trans_done:0x%08x, lcd_cmd_bits:%d, lcd_param_bits:%d, flags:{dc_as_cmd_phase:%d, dc_low_on_data:%d, octal_mode:%d, lsb_first:%d}", io_spi_config.cs_gpio_num, io_spi_config.dc_gpio_num, io_spi_config.spi_mode, io_spi_config.pclk_hz, io_spi_config.trans_queue_depth, io_spi_config.user_ctx, io_spi_config.on_color_trans_done, io_spi_config.lcd_cmd_bits, io_spi_config.lcd_param_bits, io_spi_config.flags.dc_as_cmd_phase, io_spi_config.flags.dc_low_on_data, io_spi_config.flags.octal_mode, io_spi_config.flags.lsb_first);
    esp_lcd_panel_io_handle_t io_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)AXS15231B_QSPI_HOST, &io_spi_config, &io_handle));

    // Create axs15231b panel handle
    const esp_lcd_panel_dev_config_t panel_dev_config = {
        .reset_gpio_num = AXS15231B_DEV_CONFIG_RESET_GPIO_NUM, // Verified
        .color_space = AXS15231B_DEV_CONFIG_COLOR_SPACE,
        .bits_per_pixel = AXS15231B_DEV_CONFIG_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = AXS15231B_DEV_CONFIG_FLAGS_RESET_ACTIVE_HIGH},
        .vendor_config = AXS15231B_DEV_CONFIG_VENDOR_CONFIG};
    log_d("panel_dev_config: reset_gpio_num:%d, color_space:%d, bits_per_pixel:%d, flags:{reset_active_high:%d}, vendor_config:0x%08x", panel_dev_config.reset_gpio_num, panel_dev_config.color_space, panel_dev_config.bits_per_pixel, panel_dev_config.flags.reset_active_high, panel_dev_config.vendor_config);
    esp_lcd_panel_handle_t panel_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_axs15231b(io_handle, &panel_dev_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
#ifdef DISPLAY_IPS
    // If LCD is IPS invert the colors
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif
#if (DISPLAY_SWAP_XY)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, DISPLAY_SWAP_XY));
#endif
#if (DISPLAY_MIRROR_X || DISPLAY_MIRROR_Y)
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
#endif
#if (DISPLAY_GAP_X || DISPLAY_GAP_Y)
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, DISPLAY_GAP_X, DISPLAY_GAP_Y));
#endif
    // Turn display on
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    display->user_data = panel_handle;
    display->flush_cb = axs15231b_lv_flush;

    return display;
}

#endif