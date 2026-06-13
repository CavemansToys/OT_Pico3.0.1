#include <math.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <stdlib.h>
#include <semphr.h>
#include <inttypes.h>
#include <task.h>

#include "configuration.h"
#include "scale.h"
#include "eeprom.h"
#include "app.h"
#include "scale.h"
#include "common.h"
#include "error.h"
#include "charge_mode.h"

extern charge_mode_config_t charge_mode_config;

extern scale_handle_t and_fxi_scale_handle;
extern scale_handle_t steinberg_scale_handle;
extern scale_handle_t ussolid_scale_handle;
extern scale_handle_t gng_scale_handle;
extern scale_handle_t jm_science_scale_handle;
extern scale_handle_t creedmoor_scale_handle;
extern scale_handle_t radwag_ps_r2_scale_handle;

scale_config_t scale_config;



void set_scale_driver(scale_driver_t scale_driver) {
    // Update the persistent settings
    scale_config.persistent_config.scale_driver = scale_driver;
    
    switch (scale_driver) {
        case SCALE_DRIVER_AND_FXI:
        {
            scale_config.scale_handle = &and_fxi_scale_handle;
            break;
        }
        case SCALE_DRIVER_STEINBERG_SBS:
        {
            scale_config.scale_handle = &steinberg_scale_handle;
            break;
        }
        case SCALE_DRIVER_GNG_JJB:
        {
            scale_config.scale_handle = &gng_scale_handle;
            break;
        }
        case SCALE_DRIVER_USSOLID_JFDBS:
        {
            scale_config.scale_handle = &ussolid_scale_handle;
            break;
        }
        case SCALE_DRIVER_JM_SCIENCE:
        {
            scale_config.scale_handle = &jm_science_scale_handle;
            break;
        }
        case SCALE_DRIVER_CREEDMOOR:
        {
            scale_config.scale_handle = &creedmoor_scale_handle;
            break;
        }
        case SCALE_DRIVER_RADWAG_PS_R2:
        {
            scale_config.scale_handle = &radwag_ps_r2_scale_handle;
            break;
        }
        default:
            // Invalid scale driver in EEPROM - report error and use fallback
            printf("ERROR: Invalid scale driver %d, using AND_FXI as fallback\n", scale_driver);
            report_error(ERR_SCALE_DRIVER_SELECT);
            scale_config.scale_handle = &and_fxi_scale_handle;
            break;
    }
}

uint32_t get_scale_baudrate(scale_baudrate_t scale_baudrate) {
    uint32_t baudrate_uint = 0;

    switch (scale_baudrate) {
        case BAUDRATE_4800:
            baudrate_uint = 4800;
            break;
        case BAUDRATE_9600:
            baudrate_uint = 9600;
            break;
        case BAUDRATE_19200:
            baudrate_uint = 19200;
            break;
        default:
            break;
    }

    return baudrate_uint;
}


const char * get_scale_driver_string() {
    const char * scale_driver_string = NULL;

    switch (scale_config.persistent_config.scale_driver) {
        case SCALE_DRIVER_AND_FXI:
            scale_driver_string = "AND FX-i and FZ-i";
            break;
        case SCALE_DRIVER_STEINBERG_SBS:
            scale_driver_string = "Steinberg SBS";
            break;
        case SCALE_DRIVER_USSOLID_JFDBS:
            scale_driver_string = "US Solid JFDBS";
            break;
        case SCALE_DRIVER_GNG_JJB:
            scale_driver_string = "GNG JJB";
            break;
        case SCALE_DRIVER_JM_SCIENCE:
            scale_driver_string = "JM Science";
            break;
        case SCALE_DRIVER_CREEDMOOR:
            scale_driver_string = "Creedmoor";
            break;
        case SCALE_DRIVER_RADWAG_PS_R2:
            scale_driver_string = "Radwag PS R2";
            break;
        default:
            break;
    }

    return scale_driver_string;
}


bool scale_init() {
    bool is_ok;

    // Read config from EEPROM
    is_ok = eeprom_read(EEPROM_SCALE_CONFIG_BASE_ADDR, (uint8_t *) &scale_config.persistent_config, sizeof(eeprom_scale_data_t));

    bool need_defaults = false;
    if (!is_ok) {
        printf("Unable to read from EEPROM at address %x, using defaults\n", EEPROM_SCALE_CONFIG_BASE_ADDR);
        need_defaults = true;
    } else if (scale_config.persistent_config.scale_data_rev != EEPROM_SCALE_DATA_REV) {
        need_defaults = true;
    }

    if (need_defaults) {
        scale_config.persistent_config.scale_data_rev = EEPROM_SCALE_DATA_REV;
        scale_config.persistent_config.scale_driver = SCALE_DRIVER_AND_FXI;
        scale_config.persistent_config.scale_baudrate = BAUDRATE_19200;

        // Write data back (may fail if no EEPROM)
        if (is_ok && !scale_config_save()) {
            printf("Unable to write to %x\n", EEPROM_SCALE_CONFIG_BASE_ADDR);
        }
    }

    // Initialize UART
    uart_init(SCALE_UART, get_scale_baudrate(scale_config.persistent_config.scale_baudrate));

    gpio_set_function(SCALE_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(SCALE_UART_RX, GPIO_FUNC_UART);

    // Create control variables
    // Semaphore to indicate the availability of new measurement.
    scale_config.scale_measurement_ready = xSemaphoreCreateBinary();
    if (scale_config.scale_measurement_ready == NULL) {
        report_error(ERR_SCALE_SEMAPHORE_CREATE);
        return false;
    }

    // Mutex to control the access to the serial port write
    scale_config.scale_serial_write_access_mutex = xSemaphoreCreateMutex();
    if (scale_config.scale_serial_write_access_mutex == NULL) {
        report_error(ERR_SCALE_MUTEX_CREATE);
        return false;
    }

    // Initialize the measurement variable
    scale_config.current_scale_measurement = NAN;

    // Initialize the driver handle
    printf("Scale driver: %x\n", scale_config.persistent_config.scale_driver);
    set_scale_driver(scale_config.persistent_config.scale_driver);

    // Create the Task for the listener loop
    BaseType_t task_created = xTaskCreate(scale_config.scale_handle->read_loop_task, "Scale Task", configMINIMAL_STACK_SIZE, NULL, 9, NULL);
    if (task_created != pdPASS) {
        report_error(ERR_SCALE_TASK_CREATE);
        return false;
    }

    // Register to eeprom save all
    eeprom_register_handler(scale_config_save);

    return true;
}


bool scale_config_save() {
    bool is_ok = eeprom_write(EEPROM_SCALE_CONFIG_BASE_ADDR, (uint8_t *) &scale_config.persistent_config, sizeof(eeprom_scale_data_t));
    return is_ok;
}


static inline bool _take_mutex(BaseType_t scheduler_state) {
    if (scheduler_state != taskSCHEDULER_NOT_STARTED){
        if (xSemaphoreTake(scale_config.scale_serial_write_access_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            return false;
        }
    }
    return true;
}


static inline void _give_mutex(BaseType_t scheduler_state) {
    if (scheduler_state != taskSCHEDULER_NOT_STARTED){
        xSemaphoreGive(scale_config.scale_serial_write_access_mutex);
    }
}


void scale_write(const char * command, size_t len) {
    BaseType_t scheduler_state = xTaskGetSchedulerState();

    if (!_take_mutex(scheduler_state)) {
        return;
    }

    uart_write_blocking(SCALE_UART, (uint8_t *) command, len);

    _give_mutex(scheduler_state);
}


float scale_get_current_measurement() {
    return scale_config.current_scale_measurement;
}


typedef enum {
    SIM_ZERO = 0,
    SIM_COARSE,
    SIM_COARSE_SETTLE,
    SIM_FINE,
    SIM_CHARGE_DONE,
    SIM_CUP_REMOVED,
    SIM_CUP_RETURNED,
} sim_phase_t;

static bool sim_active = false;
static float sim_weight = 0.0f;
static float sim_target = 30.0f;
static sim_phase_t sim_phase = SIM_ZERO;
static TickType_t sim_phase_start_tick = 0;
static uint16_t sim_tick_count = 0;

void scale_sim_start(float target) {
    sim_active = true;
    sim_weight = 0.0f;
    sim_target = target;
    sim_phase = SIM_ZERO;
    sim_phase_start_tick = xTaskGetTickCount();
    sim_tick_count = 0;
    printf("SIM: Started with target=%.3f\n", target);
}

void scale_sim_stop(void) {
    sim_active = false;
    sim_weight = 0.0f;
    printf("SIM: Stopped\n");
}

bool scale_sim_is_active(void) {
    return sim_active;
}

static float scale_sim_next_measurement(void) {
    vTaskDelay(pdMS_TO_TICKS(200));
    sim_tick_count++;

    switch (sim_phase) {
        case SIM_ZERO:
            sim_weight = 0.0f;
            if (sim_tick_count >= 12) {
                printf("SIM: Phase COARSE\n");
                sim_phase = SIM_COARSE;
                sim_tick_count = 0;
            }
            break;

        case SIM_COARSE: {
            float coarse_target = sim_target - 5.0f;
            float rate = coarse_target / 40.0f;
            sim_weight += rate;
            if (sim_weight >= coarse_target) {
                sim_weight = coarse_target;
                printf("SIM: Phase COARSE_SETTLE (weight=%.3f)\n", sim_weight);
                sim_phase = SIM_COARSE_SETTLE;
                sim_tick_count = 0;
            }
            break;
        }

        case SIM_COARSE_SETTLE:
            if (sim_tick_count >= 5) {
                printf("SIM: Phase FINE (weight=%.3f)\n", sim_weight);
                sim_phase = SIM_FINE;
                sim_tick_count = 0;
            }
            break;

        case SIM_FINE: {
            float rate = 0.1f;
            sim_weight += rate;
            if (sim_weight >= sim_target - 0.03f) {
                sim_weight = sim_target;
                printf("SIM: Phase CHARGE_DONE (weight=%.3f)\n", sim_weight);
                sim_phase = SIM_CHARGE_DONE;
                sim_tick_count = 0;
            }
            break;
        }

        case SIM_CHARGE_DONE:
            sim_weight = sim_target;
            if (sim_tick_count >= 25) {
                printf("SIM: Phase CUP_REMOVED\n");
                sim_phase = SIM_CUP_REMOVED;
                sim_tick_count = 0;
            }
            break;

        case SIM_CUP_REMOVED:
            sim_weight = -50.0f;
            if (sim_tick_count >= 15) {
                printf("SIM: Phase CUP_RETURNED\n");
                sim_phase = SIM_CUP_RETURNED;
                sim_tick_count = 0;
            }
            break;

        case SIM_CUP_RETURNED:
            sim_weight = sim_target;
            if (sim_tick_count >= 10) {
                printf("SIM: Cycle complete, restarting\n");
                sim_phase = SIM_ZERO;
                sim_tick_count = 0;
                sim_weight = 0.0f;
            }
            break;
    }

    scale_config.current_scale_measurement = sim_weight;
    return sim_weight;
}


/*
    Block wait for the next available measurement.

    block_time_ms set to 0 to wait indefinitely.
*/
bool scale_block_wait_for_next_measurement(uint32_t block_time_ms, float * current_measurement) {
    if (sim_active) {
        *current_measurement = scale_sim_next_measurement();
        return true;
    }

    TickType_t delay_ticks;

    if (block_time_ms == 0) {
        delay_ticks = portMAX_DELAY;
    }
    else {
        delay_ticks = pdMS_TO_TICKS(block_time_ms);
    }

    // You can only call this once the scheduler starts
    if (xSemaphoreTake(scale_config.scale_measurement_ready, delay_ticks) == pdTRUE){
        *current_measurement = scale_get_current_measurement();

        return true;
    }
    
    // No valid measurement
    return false;
}


bool http_rest_scale_config(struct fs_file *file, int num_params, char *params[], char *values[]) {
    // Mappings:
    // s0 (int): driver index
    // s1 (int): baud rate index
    // ee (bool): save to eeprom

    static char scale_config_to_json_buffer[256];
    bool save_to_eeprom = false;

    // Set value
    for (int idx = 0; idx < num_params; idx += 1) {
        if (strcmp(params[idx], "s0") == 0) {
            scale_driver_t driver_idx = (scale_driver_t) atoi(values[idx]);
            set_scale_driver(driver_idx);
        }
        else if (strcmp(params[idx], "s1") == 0) {
            scale_baudrate_t baudrate_idx = (scale_baudrate_t) atoi(values[idx]);
            scale_config.persistent_config.scale_baudrate = baudrate_idx;
        }
        else if (strcmp(params[idx], "ee") == 0) {
            save_to_eeprom = string_to_boolean(values[idx]);
        }
    }

    // Perform action
    if (save_to_eeprom) {
        scale_config_save();
    }

    snprintf(scale_config_to_json_buffer, 
             sizeof(scale_config_to_json_buffer),
             "%s"
             "{\"s0\":%d,\"s1\":%d}", 
             http_json_header,
             scale_config.persistent_config.scale_driver, 
             scale_config.persistent_config.scale_baudrate);
    
    size_t data_length = strlen(scale_config_to_json_buffer);
    file->data = scale_config_to_json_buffer;
    file->len = data_length;
    file->index = data_length;
    file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;

    return true;
}


bool http_rest_scale_action(struct fs_file *file, int num_params, char *params[], char *values[]) {
    // Mappings:
    // a0 (scale_action_t): Command to the scale
    
    // Control
    scale_action_t action = SCALE_ACTION_NO_ACTION;

    for (int idx = 0; idx < num_params; idx += 1) {
        if (strcmp(params[idx], "a0") == 0) {
            action = (scale_action_t) atoi(values[idx]);
            
            switch (action) {
                case SCALE_ACTION_FORCE_ZERO:
                    if (scale_config.scale_handle && scale_config.scale_handle->force_zero) {
                        scale_config.scale_handle->force_zero();
                    }
                    break;
                default:
                    break;
            }
        }
    }

    static char json_buffer[64];

    // Response
    snprintf(json_buffer, 
             sizeof(json_buffer),
             "%s"
             "{\"a0\":%d}",
             http_json_header,
             (int) action);

    size_t data_length = strlen(json_buffer);
    file->data = json_buffer;
    file->len = data_length;
    file->index = data_length;
    file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;

    return true;
}