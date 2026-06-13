#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "wireless.h"
#include "http_rest.h"
#include "common.h"
#include "eeprom.h"
#include "display.h"
#include "encoder.h"

#define EEPROM_WIFI_DATA_REV 2

// WiFi credentials structure for EEPROM storage
typedef struct {
    uint16_t rev;
    char home_ssid[33];
    char home_password[64];
    uint32_t auth_method;
    uint32_t timeout_ms;
    uint32_t wifi_enabled;
    uint32_t crc;
} eeprom_wifi_data_t;

// Global WiFi config - exposed for REST API
char home_ssid[33];
char home_password[64];
uint32_t home_auth_method = 2;
uint32_t home_timeout_ms = 30000;
bool home_wifi_enabled = true;

// Current WiFi status - for UI display
char wifi_ssid[33] = "Not Connected";
char wifi_ip_address[16] = "0.0.0.0";



// Load WiFi config from EEPROM on startup
bool wireless_config_init(void) {
    // Always register the save handler so eeprom_save_all() includes WiFi
    eeprom_register_handler(wireless_config_save);

    eeprom_wifi_data_t eeprom_data = {0};
    bool is_ok = eeprom_read(EEPROM_APP_CONFIG_BASE_ADDR, (uint8_t *) &eeprom_data, sizeof(eeprom_wifi_data_t));

    if (!is_ok || eeprom_data.rev != EEPROM_WIFI_DATA_REV) {
        printf("WiFi EEPROM config invalid or not found, using defaults\n");
        return false;
    }

    // Verify CRC
    uint32_t expected_crc = software_crc32(&eeprom_data, offsetof(eeprom_wifi_data_t, crc));
    if (eeprom_data.crc != expected_crc) {
        printf("WiFi EEPROM config CRC mismatch\n");
        return false;
    }

    snprintf(home_ssid, sizeof(home_ssid), "%s", eeprom_data.home_ssid);
    snprintf(home_password, sizeof(home_password), "%s", eeprom_data.home_password);
    home_auth_method = eeprom_data.auth_method;
    home_timeout_ms = eeprom_data.timeout_ms;
    home_wifi_enabled = eeprom_data.wifi_enabled;
    printf("Loaded WiFi config from EEPROM: SSID=%s\n", home_ssid);

    return true;
}

// Save WiFi config to EEPROM
bool wireless_config_save(void) {
    eeprom_wifi_data_t eeprom_data = {0};
    eeprom_data.rev = EEPROM_WIFI_DATA_REV;
    snprintf(eeprom_data.home_ssid, sizeof(eeprom_data.home_ssid), "%s", home_ssid);
    snprintf(eeprom_data.home_password, sizeof(eeprom_data.home_password), "%s", home_password);
    eeprom_data.auth_method = home_auth_method;
    eeprom_data.timeout_ms = home_timeout_ms;
    eeprom_data.wifi_enabled = home_wifi_enabled ? 1 : 0;
    eeprom_data.crc = software_crc32(&eeprom_data, offsetof(eeprom_wifi_data_t, crc));

    printf("Writing WiFi credentials to EEPROM...\n");
    bool is_ok = eeprom_write(EEPROM_APP_CONFIG_BASE_ADDR, (uint8_t *) &eeprom_data, sizeof(eeprom_wifi_data_t));
    if (is_ok) {
        printf("WiFi credentials written to EEPROM\n");
    } else {
        printf("ERROR: Failed to write WiFi credentials to EEPROM\n");
    }
    return is_ok;
}

// REST API handler for wireless configuration
bool http_rest_wireless_config(struct fs_file *file, int num_params, char *params[], char *values[]) {
    // Mapping
    // w0 (str): ssid
    // w1 (str): pw
    // w2 (int): auth
    // w3 (int): timeout_ms
    // w4 (bool): enable
    // ee (bool): save to EEPROM

    static char wireless_config_json_buffer[256];
    bool save_config = false;

    for (int idx = 0; idx < num_params; idx += 1) {
        if (strcmp(params[idx], "w0") == 0) {
            strncpy(home_ssid, values[idx], sizeof(home_ssid) - 1);
            home_ssid[sizeof(home_ssid) - 1] = '\0';
        }
        else if (strcmp(params[idx], "w1") == 0) {
            strncpy(home_password, values[idx], sizeof(home_password) - 1);
            home_password[sizeof(home_password) - 1] = '\0';
        }
        else if (strcmp(params[idx], "w2") == 0) {
            int val = atoi(values[idx]);
            if (val < 0) val = 0;
            if (val > 3) val = 3;
            home_auth_method = (uint32_t) val;
        }
        else if (strcmp(params[idx], "w3") == 0) {
            int val = atoi(values[idx]);
            if (val < 1000) val = 1000;
            if (val > 120000) val = 120000;
            home_timeout_ms = (uint32_t) val;
        }
        else if (strcmp(params[idx], "w4") == 0) {
            home_wifi_enabled = string_to_boolean(values[idx]);
        }
        else if (strcmp(params[idx], "ee") == 0) {
            save_config = string_to_boolean(values[idx]);
        }
    }

    if (save_config) {
        wireless_config_save();
    }

    // Sanitize SSID for JSON: replace any quotes/backslashes with underscores
    char safe_ssid[33];
    strncpy(safe_ssid, home_ssid, sizeof(safe_ssid) - 1);
    safe_ssid[sizeof(safe_ssid) - 1] = '\0';
    for (char *p = safe_ssid; *p; p++) {
        if (*p == '"' || *p == '\\' || *p < 0x20) {
            *p = '_';
        }
    }

    snprintf(wireless_config_json_buffer,
             sizeof(wireless_config_json_buffer),
             "%s"
             "{\"w0\":\"%.32s\",\"w2\":%"PRIu32",\"w3\":%"PRIu32",\"w4\":%s}",
             http_json_header,
             safe_ssid,
             home_auth_method,
             home_timeout_ms,
             boolean_to_string(home_wifi_enabled));

    size_t data_length = strlen(wireless_config_json_buffer);
    file->data = wireless_config_json_buffer;
    file->len = data_length;
    file->index = data_length;
    file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;

    return true;
}

// Simple LED control - use cyw43_gpio_set() directly (not cyw43_arch_gpio_put which can hang)
#define LED_GPIO 0

void pico_led_set(bool on) {
    cyw43_gpio_set(&cyw43_state, LED_GPIO, on);
}

uint8_t wireless_view_wifi_info(void) {
    u8g2_t *display_handler = get_display_handler();

    acquire_display_buffer_access();
    u8g2_ClearBuffer(display_handler);

    u8g2_SetFont(display_handler, u8g2_font_helvB08_tr);
    u8g2_DrawStr(display_handler, 5, 10, "WiFi Info");
    u8g2_DrawHLine(display_handler, 0, 13, u8g2_GetDisplayWidth(display_handler));

    u8g2_SetFont(display_handler, u8g2_font_profont11_tf);

    char buf[32];
    snprintf(buf, sizeof(buf), "SSID: %.20s", wifi_ssid);
    u8g2_DrawStr(display_handler, 3, 26, buf);

    snprintf(buf, sizeof(buf), "IP: %.15s", wifi_ip_address);
    u8g2_DrawStr(display_handler, 3, 38, buf);

    snprintf(buf, sizeof(buf), "Cfg: %.20s", home_ssid);
    u8g2_DrawStr(display_handler, 3, 50, buf);

    u8g2_SetFont(display_handler, u8g2_font_helvR08_tr);
    u8g2_DrawStr(display_handler, 45, 62, "[OK]");

    u8g2_SendBuffer(display_handler);
    release_display_buffer_access();

    while (true) {
        ButtonEncoderEvent_t event = button_wait_for_input(true);
        if (event == BUTTON_ENCODER_PRESSED || event == BUTTON_RST_PRESSED) {
            break;
        }
    }

    return 40;
}

bool http_rest_pico_led(struct fs_file *file, int num_params, char *params[], char *values[]) {
    static char pico_led_json_buffer[128];

    for (int idx = 0; idx < num_params; idx++) {
        if (strcmp(params[idx], "state") == 0) {
            bool on = (strcmp(values[idx], "1") == 0 || strcmp(values[idx], "on") == 0 || strcmp(values[idx], "true") == 0);
            cyw43_gpio_set(&cyw43_state, LED_GPIO, on);
        }
    }

    bool led_state = false;
    cyw43_gpio_get(&cyw43_state, LED_GPIO, &led_state);

    snprintf(pico_led_json_buffer, sizeof(pico_led_json_buffer),
             "%s{\"state\":%s}",
             http_json_header,
             boolean_to_string(led_state));

    size_t data_length = strlen(pico_led_json_buffer);
    file->data = pico_led_json_buffer;
    file->len = data_length;
    file->index = data_length;
    file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;

    return true;
}
