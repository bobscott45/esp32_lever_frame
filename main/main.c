#include "bsp/board.h"
#include "bsp/lvgl_port.h"
#include "lever.h"
#include "lever_frame.h"


static const lever_def_t frame1_levers[] = {
    {"UP\nDISTANT", LEVER_TYPE_DISTANT_SIGNAL},
    {"UP\nHOME", LEVER_TYPE_HOME_SIGNAL},
    {"SPARE", LEVER_TYPE_SPARE},
    {"FACING\nPOINTS", LEVER_TYPE_FACING_POINTS},
    {"SPARE", LEVER_TYPE_SPARE},
    {"SPARE", LEVER_TYPE_SPARE},
    {"SPARE", LEVER_TYPE_SPARE},
    {"SPARE", LEVER_TYPE_SPARE}
};

static const lever_def_t frame2_levers[] = {
    {"SPARE", LEVER_TYPE_SPARE},
    {"SPARE", LEVER_TYPE_SPARE},
    {"SPARE", LEVER_TYPE_SPARE},
    {"SPARE", LEVER_TYPE_SPARE}
};

static const lever_def_t frame3_levers[] = {
    {"YARD#1", LEVER_TYPE_POINTS},
    {"YARD#2", LEVER_TYPE_POINTS},
    {"MAIN LINE", LEVER_TYPE_FACING_POINTS},
    {"SPARE", LEVER_TYPE_SPARE}
};

static const tab_def_t app_tabs[] = {
    {
        .name = "North Junction Frame 1",
        .levers = frame1_levers,
        .lever_count = sizeof(frame1_levers) / sizeof(frame1_levers[0])
    },
    {
        .name = "North Junction Frame 1",
        .levers = frame2_levers,
        .lever_count = sizeof(frame2_levers) / sizeof(frame2_levers[0])
    },
    {
        .name = "Sidings",
        .levers = frame3_levers,
        .lever_count = sizeof(frame3_levers) / sizeof(frame3_levers[0])
    }
};

static const lever_system_config_t system_config = {
    .tabs = app_tabs,
    .tab_count = sizeof(app_tabs) / sizeof(app_tabs[0])
};

void app_main(void)
{
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;

    // 1. Initialize hardware drivers independently
    ESP_ERROR_CHECK(waveshare_esp32_s3_rgb_lcd_init(&panel, &touch));

    // 2. Initialize the LVGL port task loop with retrieved handles
    ESP_ERROR_CHECK(lvgl_port_init(panel, touch));

    // 3. Turn on the screen backlight
    ESP_ERROR_CHECK(waveshare_rgb_lcd_bl_on());

    // 4. Lock the port and build your UI from configuration
    if (lvgl_port_lock(-1)) {
        lever_system_create(lv_scr_act(), &system_config);
        lvgl_port_unlock();
    }
}
