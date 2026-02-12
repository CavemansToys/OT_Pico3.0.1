/**
 * @file ui_settings.cpp
 * @brief Settings Screens - Full implementation
 */

#include "lvgl_port.h"
#include "ui_common.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

// External dependencies
extern "C" {
    #include "ui_compat.h"
    #include "scale.h"
    #include "servo_gate.h"
    #include "display_config.h"
    #include <FreeRTOS.h>
    #include <queue.h>
}

// Forward declarations
extern void ui_show_main_menu(void);
extern void ui_show_ai_tuning(void);

// Screen objects
static lv_obj_t *settings_screen = NULL;
static lv_obj_t *settings_list = NULL;

// Scale settings screen objects
static lv_obj_t *scale_screen = NULL;
static lv_obj_t *scale_driver_dropdown = NULL;

// Profile manager screen objects
static lv_obj_t *profile_screen = NULL;
static lv_obj_t *profile_list = NULL;

// Display settings screen objects
static lv_obj_t *display_screen = NULL;
static lv_obj_t *brightness_slider = NULL;
static lv_obj_t *brightness_label = NULL;
static lv_obj_t *rotation_dropdown = NULL;

// Settings menu items
typedef enum {
    SETTINGS_AI_TUNING,
    SETTINGS_SCALE,
    SETTINGS_PROFILES,
    SETTINGS_EEPROM,
    SETTINGS_SERVO,
    SETTINGS_DISPLAY,
    SETTINGS_WIFI,
    SETTINGS_REBOOT,
    SETTINGS_VERSION,
    SETTINGS_BACK,
} settings_item_t;

// ============ Scale Settings Screen ============

static const char *scale_driver_names[] = {
    "A&D FX-i/FZ-i",
    "Steinberg SBS",
    "G&G JJB",
    "US Solid JFDBS",
    "JM Science",
    "Creedmoor",
    "Radwag PS-R2"
};

static void scale_back_cb(lv_event_t *e) {
    (void)e;
    ui_show_settings();
}

static void scale_driver_changed_cb(lv_event_t *e) {
    lv_obj_t *dropdown = (lv_obj_t *)lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dropdown);
    set_scale_driver((scale_driver_t)sel);
}

static void scale_calibrate_cb(lv_event_t *e) {
    (void)e;
    // Trigger scale calibration through app state
    ui_show_warning("Scale Calibration",
                   "This will start the A&D FX-i scale calibration process.\n\n"
                   "Make sure pan is empty and you have the calibration weight ready.\n\n"
                   "Continue?",
                   []() {
                       extern AppState_t exit_state;
                       exit_state = APP_STATE_ENTER_SCALE_CALIBRATION;
                       extern QueueHandle_t encoder_event_queue;
                       ButtonEncoderEvent_t event = OVERRIDE_FROM_REST;
                       xQueueSend(encoder_event_queue, &event, 0);
                   },
                   []() {
                       // Cancel - do nothing
                   });
}

static void scale_zero_cb(lv_event_t *e) {
    (void)e;
    extern scale_config_t scale_config;
    if (scale_config.scale_handle && scale_config.scale_handle->force_zero) {
        scale_config.scale_handle->force_zero();
        ui_show_toast("Scale zeroed", 1500);
    } else {
        ui_show_toast("Zero not supported for this scale", 2000);
    }
}

static void scale_save_cb(lv_event_t *e) {
    (void)e;
    if (scale_config_save()) {
        ui_show_toast("Scale settings saved!", 1500);
    } else {
        ui_show_toast("Failed to save settings", 2000);
    }
}

static void ui_scale_create(void) {
    if (scale_screen) {
        lv_obj_del(scale_screen);
    }

    scale_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scale_screen, UI_COLOR_BG, 0);

    // Title bar
    ui_create_titlebar(scale_screen, "Scale Settings");

    // Content panel
    lv_obj_t *panel = lv_obj_create(scale_screen);
    lv_obj_set_size(panel, UI_SCREEN_WIDTH - 40, 180);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 15, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Scale driver label
    lv_obj_t *driver_label = lv_label_create(panel);
    lv_label_set_text(driver_label, "Scale Driver:");
    lv_obj_set_style_text_color(driver_label, UI_COLOR_TEXT, 0);

    // Scale driver dropdown
    scale_driver_dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(scale_driver_dropdown,
        "A&D FX-i/FZ-i\n"
        "Steinberg SBS\n"
        "G&G JJB\n"
        "US Solid JFDBS\n"
        "JM Science\n"
        "Creedmoor\n"
        "Radwag PS-R2");
    lv_obj_set_width(scale_driver_dropdown, UI_SCREEN_WIDTH - 80);

    // Get current driver and set dropdown
    extern scale_config_t scale_config;
    lv_dropdown_set_selected(scale_driver_dropdown, (uint16_t)scale_config.persistent_config.scale_driver);
    lv_obj_add_event_cb(scale_driver_dropdown, scale_driver_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Spacer
    lv_obj_t *spacer = lv_obj_create(panel);
    lv_obj_set_size(spacer, 1, 10);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);

    // Current weight display
    lv_obj_t *weight_label = lv_label_create(panel);
    char weight_str[64];
    float current = scale_get_current_measurement();
    snprintf(weight_str, sizeof(weight_str), "Current Reading: %.3f gn", current);
    lv_label_set_text(weight_label, weight_str);
    lv_obj_set_style_text_color(weight_label, UI_COLOR_TEXT_SECONDARY, 0);

    // Button row
    lv_obj_t *btn_row = lv_obj_create(scale_screen);
    lv_obj_set_size(btn_row, UI_SCREEN_WIDTH - 40, 50);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Zero button
    lv_obj_t *btn_zero = ui_create_button(btn_row, "ZERO", scale_zero_cb);
    lv_obj_set_size(btn_zero, 100, 40);

    // Calibrate button
    lv_obj_t *btn_cal = ui_create_button(btn_row, "CALIBRATE", scale_calibrate_cb);
    lv_obj_set_size(btn_cal, 120, 40);

    // Save button
    lv_obj_t *btn_save = ui_create_button(btn_row, "SAVE", scale_save_cb);
    lv_obj_set_size(btn_save, 100, 40);

    // Back button
    lv_obj_t *btn_back = ui_create_button(scale_screen, "BACK", scale_back_cb);
    lv_obj_set_size(btn_back, 100, 44);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 20, -10);
}

void ui_show_scale_settings(void) {
    ui_scale_create();
    lv_scr_load(scale_screen);
}

// ============ Profile Manager Screen ============

static void profile_back_cb(lv_event_t *e) {
    (void)e;
    ui_show_settings();
}

static void profile_item_cb(lv_event_t *e) {
    uint8_t idx = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    profile_select(idx);

    profile_t *p = profile_get_selected();
    char msg[128];
    snprintf(msg, sizeof(msg), "Selected: %s", p->name);
    ui_show_toast(msg, 1500);

    // Refresh the list to show selection
    ui_show_profile_manager();
}

static void profile_save_cb(lv_event_t *e) {
    (void)e;
    if (profile_data_save()) {
        ui_show_toast("Profiles saved!", 1500);
    } else {
        ui_show_toast("Failed to save", 2000);
    }
}

static void ui_profile_create(void) {
    if (profile_screen) {
        lv_obj_del(profile_screen);
    }

    profile_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(profile_screen, UI_COLOR_BG, 0);

    // Title bar
    ui_create_titlebar(profile_screen, "Profile Manager");

    // Get current profile index
    uint16_t current_idx = profile_get_selected_idx();

    // Profile list
    profile_list = lv_list_create(profile_screen);
    lv_obj_set_size(profile_list, UI_SCREEN_WIDTH - 40, 180);
    lv_obj_align(profile_list, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(profile_list, UI_COLOR_PANEL, 0);
    lv_obj_set_style_border_width(profile_list, 0, 0);
    lv_obj_set_style_radius(profile_list, 8, 0);

    // Add profile items
    for (uint8_t i = 0; i < MAX_PROFILE_CNT; i++) {
        profile_t *p = profile_select(i);
        if (p && strlen(p->name) > 0) {
            char item_text[64];
            snprintf(item_text, sizeof(item_text), "%s%s",
                     (i == current_idx) ? LV_SYMBOL_OK " " : "   ",
                     p->name);

            lv_obj_t *btn = lv_list_add_btn(profile_list, NULL, item_text);
            lv_obj_add_event_cb(btn, profile_item_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
            lv_obj_set_style_bg_color(btn, UI_COLOR_PANEL, 0);
            lv_obj_set_style_text_color(btn, UI_COLOR_TEXT, 0);

            if (i == current_idx) {
                lv_obj_set_style_text_color(btn, UI_COLOR_SECONDARY, 0);
            }
        }
    }

    // Re-select original profile
    profile_select(current_idx);

    // Info label
    lv_obj_t *info_label = lv_label_create(profile_screen);
    profile_t *current = profile_get_selected();
    char info[128];
    snprintf(info, sizeof(info), "Current: %s | Coarse Kp:%.2f Kd:%.2f | Fine Kp:%.2f Kd:%.2f",
             current->name, current->coarse_kp, current->coarse_kd,
             current->fine_kp, current->fine_kd);
    lv_label_set_text(info_label, info);
    lv_obj_set_style_text_color(info_label, UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(info_label, UI_FONT_SMALL, 0);
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -60);

    // Back button
    lv_obj_t *btn_back = ui_create_button(profile_screen, "BACK", profile_back_cb);
    lv_obj_set_size(btn_back, 100, 44);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 20, -10);

    // Save button
    lv_obj_t *btn_save = ui_create_button(profile_screen, "SAVE", profile_save_cb);
    lv_obj_set_size(btn_save, 100, 44);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
}

void ui_show_profile_manager(void) {
    ui_profile_create();
    lv_scr_load(profile_screen);
}

// ============ Display Settings Screen ============

static void display_back_cb(lv_event_t *e) {
    (void)e;
    ui_show_settings();
}

static void brightness_changed_cb(lv_event_t *e) {
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    display_config_t *cfg = display_config_get();
    cfg->brightness = (uint8_t)value;

    // Update label
    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d%%", (value * 100) / 255);
    lv_label_set_text(brightness_label, buf);

    // Apply brightness immediately if TFT driver supports it
#ifdef USE_COLOR_TFT
    extern void tft35_set_brightness(uint8_t brightness);
    tft35_set_brightness((uint8_t)value);
#endif
}

static void rotation_changed_cb(lv_event_t *e) {
    lv_obj_t *dropdown = (lv_obj_t *)lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dropdown);

    display_config_t *cfg = display_config_get();
    cfg->rotation = (display_rotation_t)sel;
}

static void display_save_cb(lv_event_t *e) {
    (void)e;
    if (display_config_save()) {
        ui_show_toast("Display settings saved! Reboot to apply rotation.", 2500);
    } else {
        ui_show_toast("Failed to save", 2000);
    }
}

static void ui_display_create(void) {
    if (display_screen) {
        lv_obj_del(display_screen);
    }

    display_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(display_screen, UI_COLOR_BG, 0);

    // Title bar
    ui_create_titlebar(display_screen, "Display Settings");

    // Get current config
    display_config_t *cfg = display_config_get();

    // Content panel
    lv_obj_t *panel = lv_obj_create(display_screen);
    lv_obj_set_size(panel, UI_SCREEN_WIDTH - 40, 180);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 15, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Brightness label
    brightness_label = lv_label_create(panel);
    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d%%", (cfg->brightness * 100) / 255);
    lv_label_set_text(brightness_label, buf);
    lv_obj_set_style_text_color(brightness_label, UI_COLOR_TEXT, 0);

    // Brightness slider
    brightness_slider = lv_slider_create(panel);
    lv_obj_set_width(brightness_slider, UI_SCREEN_WIDTH - 100);
    lv_slider_set_range(brightness_slider, 10, 255);  // Min 10 to avoid completely dark
    lv_slider_set_value(brightness_slider, cfg->brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, brightness_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Spacer
    lv_obj_t *spacer = lv_obj_create(panel);
    lv_obj_set_size(spacer, 1, 15);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);

    // Rotation label
    lv_obj_t *rot_label = lv_label_create(panel);
    lv_label_set_text(rot_label, "Rotation:");
    lv_obj_set_style_text_color(rot_label, UI_COLOR_TEXT, 0);

    // Rotation dropdown
    rotation_dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(rotation_dropdown,
        "0\xC2\xB0 (Normal)\n"
        "90\xC2\xB0\n"
        "180\xC2\xB0 (Flipped)\n"
        "270\xC2\xB0");
    lv_obj_set_width(rotation_dropdown, UI_SCREEN_WIDTH - 100);
    lv_dropdown_set_selected(rotation_dropdown, (uint16_t)cfg->rotation);
    lv_obj_add_event_cb(rotation_dropdown, rotation_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Note about reboot
    lv_obj_t *note = lv_label_create(panel);
    lv_label_set_text(note, "Note: Rotation changes require reboot");
    lv_obj_set_style_text_color(note, UI_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(note, UI_FONT_SMALL, 0);

    // Back button
    lv_obj_t *btn_back = ui_create_button(display_screen, "BACK", display_back_cb);
    lv_obj_set_size(btn_back, 100, 44);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 20, -10);

    // Save button
    lv_obj_t *btn_save = ui_create_button(display_screen, "SAVE", display_save_cb);
    lv_obj_set_size(btn_save, 100, 44);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
}

void ui_show_display_settings(void) {
    ui_display_create();
    lv_scr_load(display_screen);
}

// ============ Main Settings Screen ============

// Back button callback
static void btn_back_cb(lv_event_t *e) {
    (void)e;
    ui_show_main_menu();
}

// List item callbacks
static void list_item_cb(lv_event_t *e) {
    settings_item_t item = (settings_item_t)(intptr_t)lv_event_get_user_data(e);

    switch (item) {
        case SETTINGS_AI_TUNING:
            ui_show_ai_tuning();
            break;

        case SETTINGS_SCALE:
            ui_show_scale_settings();
            break;

        case SETTINGS_PROFILES:
            ui_show_profile_manager();
            break;

        case SETTINGS_EEPROM:
            ui_show_warning("Save Settings",
                           "Save current settings to EEPROM?",
                           []() {
                               eeprom_save_all();
                               ui_show_toast("Settings saved!", 2000);
                           },
                           NULL);
            break;

        case SETTINGS_SERVO:
            {
                extern servo_gate_t servo_gate;
                gate_state_t state = servo_gate.gate_state;
                if (state == GATE_DISABLED) {
                    servo_gate_set_state(GATE_OPEN, true);
                    ui_show_toast("Servo Gate: Open", 1500);
                } else if (state == GATE_OPEN) {
                    servo_gate_set_state(GATE_CLOSE, true);
                    ui_show_toast("Servo Gate: Closed", 1500);
                } else {
                    servo_gate_set_state(GATE_DISABLED, true);
                    ui_show_toast("Servo Gate: Disabled", 1500);
                }
            }
            break;

        case SETTINGS_DISPLAY:
            ui_show_display_settings();
            break;

        case SETTINGS_WIFI:
            ui_show_wireless();
            break;

        case SETTINGS_REBOOT:
            ui_show_warning("Reboot",
                           "Reboot the device?",
                           []() {
                               extern AppState_t exit_state;
                               exit_state = APP_STATE_ENTER_REBOOT;
                               extern QueueHandle_t encoder_event_queue;
                               ButtonEncoderEvent_t event = BUTTON_RST_PRESSED;
                               xQueueSend(encoder_event_queue, &event, 0);
                           },
                           NULL);
            break;

        case SETTINGS_VERSION:
            {
                extern const char *version_string;
                extern const char *build_type;
                char version_msg[128];
                snprintf(version_msg, sizeof(version_msg),
                         "Version: %s\nBuild: %s",
                         version_string, build_type);
                ui_show_warning("Version Info", version_msg, NULL, NULL);
            }
            break;

        case SETTINGS_BACK:
            ui_show_main_menu();
            break;
    }
}

// Create settings screen
void ui_settings_create(void) {
    if (settings_screen) {
        lv_obj_del(settings_screen);
    }

    settings_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_screen, UI_COLOR_BG, 0);

    // Title bar
    ui_create_titlebar(settings_screen, "Settings");

    // Settings list
    settings_list = lv_list_create(settings_screen);
    lv_obj_set_size(settings_list, UI_SCREEN_WIDTH - 40, 210);
    lv_obj_align(settings_list, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_color(settings_list, UI_COLOR_PANEL, 0);
    lv_obj_set_style_border_width(settings_list, 0, 0);
    lv_obj_set_style_radius(settings_list, 8, 0);

    // Add menu items with icons
    struct {
        const char *icon;
        const char *text;
        settings_item_t item;
    } menu_items[] = {
        {LV_SYMBOL_CHARGE,    "AI Auto-Tuning",  SETTINGS_AI_TUNING},
        {LV_SYMBOL_DOWNLOAD,  "Scale",           SETTINGS_SCALE},
        {LV_SYMBOL_LIST,      "Profile Manager", SETTINGS_PROFILES},
        {LV_SYMBOL_SAVE,      "Save to EEPROM",  SETTINGS_EEPROM},
        {LV_SYMBOL_POWER,     "Servo Gate",      SETTINGS_SERVO},
        {LV_SYMBOL_IMAGE,     "Display",         SETTINGS_DISPLAY},
        {LV_SYMBOL_WIFI,      "WiFi Info",       SETTINGS_WIFI},
        {LV_SYMBOL_LOOP,      "Reboot",          SETTINGS_REBOOT},
        {LV_SYMBOL_HOME,      "Version Info",    SETTINGS_VERSION},
    };

    for (size_t i = 0; i < sizeof(menu_items) / sizeof(menu_items[0]); i++) {
        lv_obj_t *btn = lv_list_add_btn(settings_list, menu_items[i].icon, menu_items[i].text);
        lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, (void *)(intptr_t)menu_items[i].item);
        lv_obj_set_style_bg_color(btn, UI_COLOR_PANEL, 0);
        lv_obj_set_style_text_color(btn, UI_COLOR_TEXT, 0);
    }

    // Back button
    lv_obj_t *btn_back = ui_create_button(settings_screen, "BACK", btn_back_cb);
    lv_obj_set_size(btn_back, 100, 44);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 20, -15);
}

void ui_settings_show(void) {
    ui_settings_create();
    lv_scr_load(settings_screen);
}

// Public interface
void ui_show_settings(void) {
    ui_settings_show();
}

// ============ Warning/Message Box Screen ============

static lv_obj_t *msgbox = NULL;
static void (*confirm_callback)(void) = NULL;
static void (*cancel_callback)(void) = NULL;

static void msgbox_confirm_cb(lv_event_t *e) {
    (void)e;
    if (confirm_callback) {
        confirm_callback();
    }
    if (msgbox) {
        lv_msgbox_close(msgbox);
        msgbox = NULL;
    }
    confirm_callback = NULL;
    cancel_callback = NULL;
}

static void msgbox_cancel_cb(lv_event_t *e) {
    (void)e;
    if (cancel_callback) {
        cancel_callback();
    }
    if (msgbox) {
        lv_msgbox_close(msgbox);
        msgbox = NULL;
    }
    confirm_callback = NULL;
    cancel_callback = NULL;
}

void ui_show_warning(const char *title, const char *message, void (*on_confirm)(void), void (*on_cancel)(void)) {
    if (msgbox) {
        lv_msgbox_close(msgbox);
    }

    confirm_callback = on_confirm;
    cancel_callback = on_cancel;

    msgbox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(msgbox, title);
    lv_msgbox_add_text(msgbox, message);
    lv_msgbox_add_close_button(msgbox);

    if (on_confirm || on_cancel) {
        lv_obj_t *btn_yes = lv_msgbox_add_footer_button(msgbox, "Yes");
        lv_obj_add_event_cb(btn_yes, msgbox_confirm_cb, LV_EVENT_CLICKED, NULL);

        if (on_cancel) {
            lv_obj_t *btn_no = lv_msgbox_add_footer_button(msgbox, "No");
            lv_obj_add_event_cb(btn_no, msgbox_cancel_cb, LV_EVENT_CLICKED, NULL);
        }
    } else {
        lv_obj_t *btn_ok = lv_msgbox_add_footer_button(msgbox, "OK");
        lv_obj_add_event_cb(btn_ok, msgbox_confirm_cb, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_center(msgbox);
}

// ============ Cleanup Warning Screen ============

void ui_show_cleanup_warning(void) {
    ui_show_warning("Warning",
                   "Put pan on the scale and press Yes to start cleanup mode.",
                   []() {
                       extern AppState_t exit_state;
                       exit_state = APP_STATE_ENTER_CLEANUP_MODE;
                       extern QueueHandle_t encoder_event_queue;
                       ButtonEncoderEvent_t event = OVERRIDE_FROM_REST;
                       xQueueSend(encoder_event_queue, &event, 0);
                   },
                   []() {
                       ui_show_main_menu();
                   });
}

// ============ Charge Warning Screen ============

void ui_show_charge_warning(void) {
    ui_show_warning("Warning",
                   "Put pan on the scale and press Yes to start charging.",
                   []() {
                       extern AppState_t exit_state;
                       exit_state = APP_STATE_ENTER_CHARGE_MODE;
                       extern QueueHandle_t encoder_event_queue;
                       ButtonEncoderEvent_t event = OVERRIDE_FROM_REST;
                       xQueueSend(encoder_event_queue, &event, 0);
                   },
                   []() {
                       ui_show_main_menu();
                   });
}

// ============ Wireless Screen ============

void ui_show_wireless(void) {
    extern char wifi_ip_address[];
    extern char wifi_ssid[];

    char info[128];
    snprintf(info, sizeof(info), "SSID: %s\nIP: %s", wifi_ssid, wifi_ip_address);

    ui_show_warning("WiFi Info", info, NULL, NULL);
}
