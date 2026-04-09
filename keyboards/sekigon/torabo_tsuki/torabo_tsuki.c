// Copyright 2024 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <string.h>
#include "debug.h"

#include QMK_KEYBOARD_H
#include "matrix.h"
#include "bmp.h"
#include "state_controller.h"
#include "bmp_matrix.h"
#include "spi.h"

#include "report.h"
#include "pointing_device.h"

#include "trackball.h"

#define MODE_COUNT 4
#define MAX_INTERVAL_MASTER 150
#define MAX_INTERVAL_SLAVE 900
#define BLE_INTERVAL_MASTER_0 15
#define BLE_INTERVAL_SLAVE_0 30
#define BLE_INTERVAL_TB_0 (BLE_INTERVAL_MASTER_0*2)

#define BLE_INTERVAL_MASTER_1 10
#define BLE_INTERVAL_SLAVE_1 20
#define BLE_INTERVAL_TB_1 (BLE_INTERVAL_MASTER_1*3)

#define BLE_INTERVAL_MASTER_2 10
#define BLE_INTERVAL_SLAVE_2 10
#define BLE_INTERVAL_TB_2 (BLE_INTERVAL_MASTER_2*2)

#define BLE_INTERVAL_MASTER_3 10
#define BLE_INTERVAL_SLAVE_3 10
#define BLE_INTERVAL_TB_3 (BLE_INTERVAL_MASTER_2*1)

eeconfig_kb_t eeconfig_kb;

static bool trackball_initialized;

bmp_api_ble_conn_param_t get_periph_conn_param(uint8_t mode) {
    const bmp_api_ble_conn_param_t param[MODE_COUNT] = {
        {.max_interval = BLE_INTERVAL_MASTER_0, .min_interval = BLE_INTERVAL_MASTER_0, .slave_latency = MAX_INTERVAL_MASTER / BLE_INTERVAL_MASTER_0}, //
        {.max_interval = BLE_INTERVAL_MASTER_1, .min_interval = BLE_INTERVAL_MASTER_1, .slave_latency = MAX_INTERVAL_MASTER / BLE_INTERVAL_MASTER_1}, //
        {.max_interval = BLE_INTERVAL_MASTER_2, .min_interval = BLE_INTERVAL_MASTER_2, .slave_latency = MAX_INTERVAL_MASTER / BLE_INTERVAL_MASTER_2}, //
        {.max_interval = BLE_INTERVAL_MASTER_3, .min_interval = BLE_INTERVAL_MASTER_3, .slave_latency = MAX_INTERVAL_MASTER / BLE_INTERVAL_MASTER_3}, //

    };
    return mode < MODE_COUNT ? param[mode]
                             : (bmp_api_ble_conn_param_t){MAX(eeconfig_kb.battery.custom.periph_interval, 8), //
                                                          MAX(eeconfig_kb.battery.custom.periph_interval, 8), //
                                                          eeconfig_kb.battery.custom.periph_sl};
}

bmp_api_ble_conn_param_t get_central_conn_param(uint8_t mode) {
    const bmp_api_ble_conn_param_t param[MODE_COUNT] = {
        {.max_interval = 3 * BLE_INTERVAL_SLAVE_0, .min_interval = BLE_INTERVAL_SLAVE_0, .slave_latency = MAX_INTERVAL_SLAVE / BLE_INTERVAL_SLAVE_0}, //
        {.max_interval = 2 * BLE_INTERVAL_SLAVE_1, .min_interval = BLE_INTERVAL_SLAVE_1, .slave_latency = MAX_INTERVAL_SLAVE / BLE_INTERVAL_SLAVE_1}, //
        {.max_interval = 2 * BLE_INTERVAL_SLAVE_2, .min_interval = BLE_INTERVAL_SLAVE_2, .slave_latency = MAX_INTERVAL_SLAVE / BLE_INTERVAL_SLAVE_2}, //
        {.max_interval = 1 * BLE_INTERVAL_SLAVE_3, .min_interval = BLE_INTERVAL_SLAVE_3, .slave_latency = MAX_INTERVAL_SLAVE / BLE_INTERVAL_SLAVE_3}, //

    };
    return param[mode % MODE_COUNT];
}

static uint32_t get_interval_tb(uint8_t mode) {
    const uint32_t param[MODE_COUNT] = {
        BLE_INTERVAL_TB_0,
        BLE_INTERVAL_TB_1,
        BLE_INTERVAL_TB_2,
        BLE_INTERVAL_TB_3,
    };
    return mode < MODE_COUNT ? param[mode] : param[0];
}

void keyboard_post_init_kb(void) {
    debug_enable = false;
    eeconfig_read_kb_datablock(&eeconfig_kb);

    if (eeconfig_kb.version != EECONFIG_KB_VERSION) {
        eeconfig_init_kb_datablock();
    }
    battery_type_init(eeconfig_kb.battery.type);
    pointing_device_set_cpi((eeconfig_kb.cursor.cpi_200 + 1) * 200);
    set_auto_mouse_enable(eeconfig_kb.aml.options.enable);
    set_auto_mouse_layer(eeconfig_kb.aml.layer);
    set_auto_mouse_timeout(eeconfig_kb.aml.timeout);
    keyboard_post_init_user();

    bmp_api_config_t new_config = *bmp_config;
    new_config.param_central    = get_central_conn_param(eeconfig_kb.battery.mode);
    new_config.param_peripheral = get_periph_conn_param(eeconfig_kb.battery.mode);
    BMPAPI->app.set_config(&new_config);

    static char status[8];
    if (is_keyboard_master()) {
        snprintf(status, sizeof(status), "%s", trackball_initialized ? "OK" : "NG");
        BMPAPI->usb.create_file("STATUS  TXT", (uint8_t *)status, strlen(status));
    }
}

void matrix_init_kb() {
    gpio_set_pin_input_high(CONFIG_HALF_DUPLEX_PIN);
    gpio_set_pin_input_high(TB_MOTION);

    // turn on trackball and sr
    gpio_write_pin_high(TB_POW);
    gpio_write_pin_high(SR_POW);
    gpio_set_pin_output_push_pull(TB_POW);
    gpio_set_pin_output_push_pull(SR_POW);
    gpio_write_pin_high(CS_PIN_TB0);
    gpio_write_pin_low(CONFIG_SCK_PIN);
    gpio_set_pin_output_push_pull(CS_PIN_TB0);
    gpio_set_pin_output_push_pull(CONFIG_SCK_PIN);

    // reset io expanders
    gpio_set_pin_output_push_pull(IO_RESET);
    gpio_write_pin_low(IO_RESET);

    gpio_write_pin_high(SR_DATA);
    gpio_write_pin_low(SR_CLK);
    gpio_set_pin_output_push_pull(SR_DATA);
    gpio_set_pin_output_push_pull(SR_CLK);

    // deassert reset
    gpio_write_pin_high(IO_RESET);

    gpio_write_pin_high(SR_DATA);
    wait_us(1);
    for (int idx = 0; idx < 16; idx++) {
        gpio_write_pin_high(SR_CLK);
        gpio_write_pin_low(SR_CLK);
    }

    matrix_init_user();
}

extern matrix_row_t  matrix[MATRIX_ROWS];
void                 matrix_scan_kb(void) {
    // assign TB_MOTION to matrix
    // This enable PERMISSIVE_HOLD with trackball move
    matrix[0] &= ~(1 << (MATRIX_COLS_DEFAULT - 1));
    if (is_auto_mouse_active()) {
        matrix[0] |= ((readPin(TB_MOTION) ? 0 : 1) << (MATRIX_COLS_DEFAULT - 1));
    }
    matrix_scan_user();
}

static trackball_info_t tb_info;

const trackball_info_t *get_trackball_info() { return &tb_info; }

void bmp_before_sleep(void) {
    // turn off indicator
    setPinDefault(bmp_config->indicator_led);
    // turn off trackball
    trackball_shutdown();

    // clear all cols
    gpio_write_pin_low(IO_RESET);
    gpio_write_pin_high(IO_RESET);
}

void bmp_before_shutdown(void) {
    setPinDefault(bmp_config->indicator_led);
    for (int row = 0; row < bmp_config->matrix.device_rows; row++) {
        setPinDefault(bmp_config->matrix.row_pins[row]);
    }
    trackball_shutdown();
}

bool checkSafemodeFlag(bmp_api_config_t const *const config) { return false; }

bool bmp_config_overwrite(bmp_api_config_t const *const config_on_storage,
                          bmp_api_config_t *const       keyboard_config) {
    // User can overwrite partial settings
    bmp_api_config_t         new_config  = default_config;
    bmp_api_ble_conn_param_t conn_master = get_periph_conn_param(0);
    bmp_api_ble_conn_param_t conn_slave  = get_central_conn_param(0);

    new_config.mode                = config_on_storage->mode;
    new_config.startup             = config_on_storage->startup;
    new_config.matrix.debounce     = MAX(BMP_DEBOUNCE, config_on_storage->matrix.debounce);
    new_config.matrix.is_left_hand = config_on_storage->matrix.is_left_hand;
    new_config.param_peripheral    = config_on_storage->mode == SPLIT_SLAVE ? conn_slave : conn_master;
    new_config.param_central       = conn_slave;
    new_config.reserved[2]         = config_on_storage->reserved[2];
    memset(&new_config.encoder, 0, sizeof(new_config.encoder));
    *keyboard_config = new_config;
    return true;
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    bool cont = process_record_bmp(keycode, record);

    if (cont) {
        cont = process_record_user(keycode, record);
    }

    return cont;
}

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[0]);
    if (*command_id == id_set_keyboard_value) {
        // This version does not handle save flag
    } else if (*command_id == id_unhandled) {
        via_custom_value_command_kb(&data[1], length - 1);
    } else {
        *command_id = id_unhandled;
    }
}

void pointing_device_driver_init(void) {
    trackball_initialized = (trackball_init(CS_PIN_TB0) == 0);
}

static uint32_t       rate_limit_start = 0;
static report_mouse_t mouse_report_buffer;
static bool           mouse_buffer_empty     = false;
static bool           pointing_device_active = false;

bool is_pointing_device_active(void) {
    return pointing_device_active;
}

report_mouse_t pointing_device_driver_get_report(report_mouse_t mouse_report) {
    report_mouse_t report     = mouse_report;
    bool           force_send = false;
    pointing_device_active    = true;

    uint8_t motion_pin_state = readPin(TB_MOTION);

    if (motion_pin_state == 0 && is_keyboard_master()) {
        trackball_data_t data = trackball_get();
        if (data.stat & 0x80) {
            if (bmp_config->matrix.is_left_hand) {
                report.x = data.x;
                report.y = -data.y;
            } else {
                report.x = -data.x;
                report.y = data.y;
            }
        }

        if (debug_mouse) {
            printf("%8lu %02x %6d %6d\n", timer_read32(), data.stat, data.x, data.y);
        }

        force_send                  = (mouse_report_buffer.buttons != mouse_report.buttons);
        mouse_report_buffer.buttons = report.buttons;
        mouse_report_buffer.x += report.x;
        mouse_report_buffer.y += report.y;
        mouse_report_buffer.h += report.h;
        mouse_report_buffer.v += report.v;
    }

    if (!mouse_buffer_empty && timer_elapsed32(rate_limit_start) >= get_interval_tb(eeconfig_kb.battery.mode)) {
        report                      = mouse_report_buffer;
        report.buttons              = mouse_report.buttons;
        rate_limit_start            = timer_read32();
        mouse_report_buffer         = (report_mouse_t){0};
        mouse_report_buffer.buttons = report.buttons;
        mouse_buffer_empty          = true;
        bmp_set_enable_task_interval_stretch(false);
        return report;
    } else if (motion_pin_state == 1 || !is_keyboard_master()) {
        pointing_device_active = false;
        return mouse_report;
    } else {
        // Limit send rate to prevent BLE buffer overflow
        if (force_send || !get_ble_enabled() || timer_elapsed32(rate_limit_start) >= get_interval_tb(eeconfig_kb.battery.mode)) {
            report                      = mouse_report_buffer;
            rate_limit_start            = timer_read32();
            mouse_report_buffer         = (report_mouse_t){0};
            mouse_report_buffer.buttons = report.buttons;
            mouse_buffer_empty          = true;
        } else {
            mouse_report_buffer = mouse_report;
            mouse_buffer_empty  = false;
        }

        bmp_set_enable_task_interval_stretch(false);
    }

    return report;
}

uint16_t pointing_device_driver_get_cpi(void) {
    return trackball_get_cpi();
}

void pointing_device_driver_set_cpi(uint16_t cpi) {
    trackball_set_cpi(cpi);
}

void bootmagic_lite(void) {
    matrix_scan();
    wait_ms(100);
    matrix_scan();

    uint8_t row = BOOTMAGIC_LITE_ROW;
    uint8_t col = BOOTMAGIC_LITE_COLUMN;

    if (matrix_get_row(row) & (1 << col)) {
        bootloader_jump();
    }
}