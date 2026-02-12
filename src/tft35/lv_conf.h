/**
 * @file lv_conf.h
 * @brief LVGL Configuration for TFT35/TFT43 on RP2350 (Pico 2)
 *
 * Optimized for:
 * - RP2350 Cortex-M33 with FPU
 * - 480x320 (TFT35) or 480x272 (TFT43) display
 * - FreeRTOS
 * - Minimal memory footprint
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1

/*====================
   MEMORY SETTINGS
 *====================*/

/* RP2350 has 520KB SRAM - allocate 16KB for LVGL (minimal) */
#define LV_MEM_SIZE (16 * 1024U)
#define LV_MEM_ADR 0
#define LV_MEM_CUSTOM 0
#define LV_USE_MEM_MONITOR 0

/*====================
   HAL SETTINGS
 *====================*/

#define LV_DEF_REFR_PERIOD 16      /* ~60fps */
#define LV_DPI_DEF 130

/* FreeRTOS tick source */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "FreeRTOS.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (xTaskGetTickCount() * portTICK_PERIOD_MS)
#endif

/*====================
   DRAWING
 *====================*/

#define LV_DISPLAY_DEF_REFR_PERIOD 16
#define LV_USE_DRAW_SW 1

/* RP2350 optimizations */
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB888 0
#define LV_DRAW_SW_SUPPORT_ARGB8888 0

/* Disable assembly optimizations (Helium/NEON not available on Cortex-M33) */
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE
#define LV_USE_NATIVE_HELIUM_ASM 0

/* Disable all GPU - RP2350 has none */
#define LV_USE_DRAW_ARM2D 0
#define LV_USE_DRAW_VGLITE 0
#define LV_USE_DRAW_PXP 0
#define LV_USE_DRAW_SDL 0

/*====================
   LOGGING - DISABLED FOR RELEASE
 *====================*/

#define LV_USE_LOG 0

/*====================
   ASSERTS - MINIMAL FOR RELEASE
 *====================*/

#define LV_USE_ASSERT_NULL          0
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*====================
   FEATURES
 *====================*/

#define LV_USE_ANIM 1
#define LV_USE_SHADOW 0            /* Disable shadows - saves CPU */
#define LV_SHADOW_CACHE_SIZE 0
#define LV_USE_BLEND_MODES 0
#define LV_ENABLE_GC 0

/*====================
   COMPILER - RP2350 CORTEX-M33 OPTIMIZATIONS
 *====================*/

#define LV_BIG_ENDIAN_SYSTEM 0

/* Place critical functions in RAM for speed */
#define LV_ATTRIBUTE_FAST_MEM __attribute__((section(".time_critical")))

#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_ATTRIBUTE_EXTERN_DATA

/* Use hardware FPU on RP2350 */
#define LV_USE_FLOAT 0             /* Keep integer math - faster for UI */

/*====================
   FONTS - ONLY WHAT'S NEEDED
 *====================*/

#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1    /* UI_FONT_SMALL */
#define LV_FONT_MONTSERRAT_14 1    /* UI_FONT_NORMAL (default) */
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 1    /* UI_FONT_MEDIUM */
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1    /* UI_FONT_LARGE */
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1    /* UI_FONT_XLARGE */
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_USE_FONT_SUBPX 0
#define LV_USE_FONT_PLACEHOLDER 1

/*====================
   TEXT
 *====================*/

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*====================
   WIDGETS - ONLY WHAT'S USED
 *====================*/

#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     0
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMAGE      1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     0        /* Not used */
#define LV_USE_SLIDER     1
#define LV_USE_SPAN       0
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   0        /* Not used */
#define LV_USE_TABLE      0        /* Not used */

/*====================
   EXTRA WIDGETS - MINIMAL
 *====================*/

#define LV_USE_ANIMIMG    0
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN     0
#define LV_USE_KEYBOARD   0        /* Not used */
#define LV_USE_LED        1        /* Status LED in charge mode */
#define LV_USE_LIST       1
#define LV_USE_MENU       0        /* Not used */
#define LV_USE_METER      0
#define LV_USE_MSGBOX     1
#define LV_USE_SPINBOX    0        /* Not used */
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0

/*====================
   THEMES - MINIMAL
 *====================*/

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 1
    #define LV_THEME_DEFAULT_GROW 0
    #define LV_THEME_DEFAULT_TRANSITION_TIME 50
#endif

#define LV_USE_THEME_SIMPLE 0      /* Not needed with default */

/*====================
   LAYOUTS
 *====================*/

#define LV_USE_FLEX 1
#define LV_USE_GRID 0              /* Not used */

/*====================
   OS - NONE (we handle ticks manually)
 *====================*/

#define LV_USE_OS LV_OS_NONE

/*====================
   LIBS - ALL DISABLED
 *====================*/

#define LV_USE_SNAPSHOT 0
#define LV_USE_MONKEY 0
#define LV_USE_PROFILER 0
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0
#define LV_USE_PNG 0
#define LV_USE_BMP 0
#define LV_USE_SJPG 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0
#define LV_USE_FREETYPE 0
#define LV_USE_TINY_TTF 0
#define LV_USE_RLOTTIE 0
#define LV_USE_FFMPEG 0

#endif /* LV_CONF_H */
