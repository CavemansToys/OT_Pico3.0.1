/**
 * @file ui_compat.h
 * @brief Compatibility header for LVGL UI - maps old API to new
 */

#ifndef UI_COMPAT_H
#define UI_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "charge_mode.h"
#include "profile.h"
#include "app.h"
#include "app_state.h"  // For exit_state
#include "eeprom.h"
#include "encoder.h"  // For ButtonEncoderEvent_t
#include "version.h"

// Global externs
extern charge_mode_config_t charge_mode_config;

// API Compatibility macros/inlines
static inline profile_t* get_selected_profile(void) { return profile_get_selected(); }
static inline void select_profile(uint8_t idx) { profile_select(idx); }
static inline eeprom_profile_data_t* get_profile_data(void) { extern eeprom_profile_data_t profile_data; return &profile_data; }
static inline int get_profile_count(void) { return MAX_PROFILE_CNT; }
static inline void set_charge_weight(float weight) { charge_mode_config.target_charge_weight = weight; }

// LVGL 9.x returns void* from event functions - need casts
#define LV_EVENT_GET_TARGET(e) ((lv_obj_t*)lv_event_get_target(e))
#define LV_EVENT_GET_CURRENT_TARGET(e) ((lv_obj_t*)lv_event_get_current_target(e))

#ifdef __cplusplus
}
#endif

#endif // UI_COMPAT_H
