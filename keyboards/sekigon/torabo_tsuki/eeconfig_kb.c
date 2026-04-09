// Copyright 2024 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include QMK_KEYBOARD_H
#include "eeconfig_kb.h"
#include "bmp.h"
#include "bmp_settings.h"

#include "vial_generated_custom_menu_definition.h"

void eeconfig_init_kb_datablock(void) {
    memset(&eeconfig_kb, 0, sizeof(eeconfig_kb));
    eeconfig_kb.version                        = EECONFIG_KB_VERSION;
    eeconfig_kb.cursor.cpi_200                 = (400 / 200) - 1;
    eeconfig_kb.cursor.fine_layer              = 0;
    eeconfig_kb.cursor.fine_div                = 1;
    eeconfig_kb.cursor.rough_layer             = 0;
    eeconfig_kb.cursor.rough_mul               = 1;
    eeconfig_kb.cursor.rotate                  = 0;
    eeconfig_kb.cursor.curve.shift_point[0]    = 0;
    eeconfig_kb.cursor.curve.shift_rate[0]     = 100;
    eeconfig_kb.cursor.curve.shift_point[1]    = 0;
    eeconfig_kb.cursor.curve.shift_rate[1]     = 100;
    eeconfig_kb.cursor.curve.shift_point[2]    = 0;
    eeconfig_kb.cursor.curve.shift_rate[2]     = 100;
    eeconfig_kb.aml.timeout                    = 100;
    eeconfig_kb.aml.options.enable             = false;
    eeconfig_kb.aml.layer                      = 1;
    eeconfig_kb.aml.debounce                   = 25;
    eeconfig_kb.aml.threshold                  = 10;
    eeconfig_kb.aml.delay                      = 200;
    eeconfig_kb.scroll.divide                  = 1;
    eeconfig_kb.battery.custom.periph_interval = 10;
    eeconfig_kb.battery.custom.periph_sl       = 15;
    eeconfig_kb.pseudo_encoder.divide          = 50;
    eeconfig_update_kb_datablock(&eeconfig_kb);
}

__attribute__((weak)) void via_custom_value_command_user(uint8_t *data, uint8_t length) {}

#define BYTE_ACCESS(x) (*(uint8_t*)&(x))

typedef void (*eeconfig_kb_hook)(int32_t);
typedef struct {
    uint32_t id;
    uint32_t offset;
    uint8_t size;
    eeconfig_kb_hook initialize;
} eeconfig_kb_member_t;
#define GET_OFFSET(member) (offsetof(eeconfig_kb_t, member))

static void cpi_200_init(int32_t c) {
    pointing_device_set_cpi(c * 200 + 200);
}
static void aml_option_init(int32_t _) {
    set_auto_mouse_enable(eeconfig_kb.aml.options.enable);
}
static void aml_layer_init(int32_t _) {
    set_auto_mouse_layer(eeconfig_kb.aml.layer);
}
static void aml_timeout_init(int32_t _) {
    set_auto_mouse_timeout(eeconfig_kb.aml.timeout);
}
static void aml_debounce_init(int32_t _) {
    set_auto_mouse_debounce(eeconfig_kb.aml.debounce);
}
uint16_t get_auto_mouse_delay(void) {
    return eeconfig_kb.aml.delay;
}
uint8_t get_auto_mouse_threshold(void) {
    return eeconfig_kb.aml.threshold;
}

void battery_type_init(int32_t _) {
    if (eeconfig_kb.battery.type == 0) {
        BMPAPI->adc.config_vcc_channel(BAT_IN, 1300, 1050);
    } else {
        BMPAPI->adc.config_vcc_channel(BAT_IN, 1500, 1000);
    }
}

static const eeconfig_kb_member_t eeconfig_kb_members[] = {
    [vial_custom_menu_id_trackball_cpi]                  = {.id = vial_custom_menu_id_trackball_cpi, .offset = GET_OFFSET(cursor.cpi_200), .size = 1, .initialize = cpi_200_init}, //
    [vial_custom_menu_id_trackball_fine_layer]           = {.id = vial_custom_menu_id_trackball_fine_layer, .offset = GET_OFFSET(cursor.fine_layer), .size = 1},
    [vial_custom_menu_id_trackball_fine_coef]            = {.id = vial_custom_menu_id_trackball_fine_coef, .offset = GET_OFFSET(cursor.fine_div), .size = 1},
    [vial_custom_menu_id_trackball_rough_layer]          = {.id = vial_custom_menu_id_trackball_rough_layer, .offset = GET_OFFSET(cursor.rough_layer), .size = 1},
    [vial_custom_menu_id_trackball_rough_coef]           = {.id = vial_custom_menu_id_trackball_rough_coef, .offset = GET_OFFSET(cursor.rough_mul), .size = 1},
    [vial_custom_menu_id_trackball_rotate]               = {.id = vial_custom_menu_id_trackball_rotate, .offset = GET_OFFSET(cursor.rotate), .size = 1},
    [vial_custom_menu_id_trackball_aml_enabled]          = {.id = vial_custom_menu_id_trackball_aml_enabled, .offset = GET_OFFSET(aml.options), .size = 1, .initialize = aml_option_init},
    [vial_custom_menu_id_trackball_aml_layer]            = {.id = vial_custom_menu_id_trackball_aml_layer, .offset = GET_OFFSET(aml.layer), .size = 1, .initialize = aml_layer_init},
    [vial_custom_menu_id_trackball_aml_timeout]          = {.id = vial_custom_menu_id_trackball_aml_timeout, .offset = GET_OFFSET(aml.timeout), .size = 2, .initialize = aml_timeout_init},
    [vial_custom_menu_id_trackball_aml_debounce]         = {.id = vial_custom_menu_id_trackball_aml_debounce, .offset = GET_OFFSET(aml.debounce), .size = 1, .initialize = aml_debounce_init},
    [vial_custom_menu_id_trackball_aml_threshold]        = {.id = vial_custom_menu_id_trackball_aml_threshold, .offset = GET_OFFSET(aml.threshold), .size = 1},                         //
    [vial_custom_menu_id_trackball_aml_delay]            = {.id = vial_custom_menu_id_trackball_aml_delay, .offset = GET_OFFSET(aml.delay), .size = 2},                                 //
    [vial_custom_menu_id_trackball_scroll_layer]         = {.id = vial_custom_menu_id_trackball_scroll_layer, .offset = GET_OFFSET(scroll.layer), .size = 1},                           //
    [vial_custom_menu_id_trackball_scroll_option]        = {.id = vial_custom_menu_id_trackball_scroll_option, .offset = GET_OFFSET(scroll.options), .size = 1},                        //
    [vial_custom_menu_id_trackball_scroll_divide]        = {.id = vial_custom_menu_id_trackball_scroll_divide, .offset = GET_OFFSET(scroll.divide), .size = 1},                         //
    [vial_custom_menu_id_battery_type]                   = {.id = vial_custom_menu_id_battery_type, .offset = GET_OFFSET(battery.type), .size = 1},                                     //
    [vial_custom_menu_id_battery_mode]                   = {.id = vial_custom_menu_id_battery_mode, .offset = GET_OFFSET(battery.mode), .size = 1},                                     //
    [vial_custom_menu_id_battery_custom_periph_interval] = {.id = vial_custom_menu_id_battery_custom_periph_interval, .offset = GET_OFFSET(battery.custom.periph_interval), .size = 1}, //
    [vial_custom_menu_id_battery_custom_periph_latency]  = {.id = vial_custom_menu_id_battery_custom_periph_latency, .offset = GET_OFFSET(battery.custom.periph_sl), .size = 1},        //
    [vial_custom_menu_id_encoder_layer]                  = {.id = vial_custom_menu_id_encoder_layer, .offset = GET_OFFSET(pseudo_encoder.layer), .size = 1},                            //
    [vial_custom_menu_id_encoder_snap]                   = {.id = vial_custom_menu_id_encoder_snap, .offset = GET_OFFSET(pseudo_encoder.snap_layer), .size = 1},                        //
    [vial_custom_menu_id_encoder_divide]                 = {.id = vial_custom_menu_id_encoder_divide, .offset = GET_OFFSET(pseudo_encoder.divide), .size = 1},                          //
    [vial_custom_menu_id_curve_shift_point_0_]           = {.id = vial_custom_menu_id_curve_shift_point_0_, .offset = GET_OFFSET(cursor.curve.shift_point[0]), .size = 1},              //
    [vial_custom_menu_id_curve_shift_rate_0_]            = {.id = vial_custom_menu_id_curve_shift_rate_0_, .offset = GET_OFFSET(cursor.curve.shift_rate[0]), .size = 1},                //
    [vial_custom_menu_id_curve_shift_point_1_]           = {.id = vial_custom_menu_id_curve_shift_point_1_, .offset = GET_OFFSET(cursor.curve.shift_point[1]), .size = 1},              //
    [vial_custom_menu_id_curve_shift_rate_1_]            = {.id = vial_custom_menu_id_curve_shift_rate_1_, .offset = GET_OFFSET(cursor.curve.shift_rate[1]), .size = 1},                //
    [vial_custom_menu_id_curve_shift_point_2_]           = {.id = vial_custom_menu_id_curve_shift_point_2_, .offset = GET_OFFSET(cursor.curve.shift_point[2]), .size = 1},              //
    [vial_custom_menu_id_curve_shift_rate_2_]            = {.id = vial_custom_menu_id_curve_shift_rate_2_, .offset = GET_OFFSET(cursor.curve.shift_rate[2]), .size = 1},                //
};

static void process_custom_value_command(uint8_t *data, uint8_t length) {
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*command_id == id_eeprom_reset) {
        eeconfig_init_kb_datablock();
        return;
    }

    if (*channel_id != id_custom_channel) {
        return;
    }

    if (value_id_and_data[0] >= sizeof(eeconfig_kb_members) / sizeof(eeconfig_kb_members[0])) {
        return;
    }

    const eeconfig_kb_member_t *member = NULL;

    for (int idx = 0; idx < sizeof(eeconfig_kb_members) / sizeof(eeconfig_kb_members[0]); idx++) {
        if (eeconfig_kb_members[idx].id == value_id_and_data[0]) {
            member = &eeconfig_kb_members[idx];
            break;
        }
    }

    if (member == NULL) {
        return;
    }

    if (member->size == 0) {
        return;
    }

    if (*command_id == id_custom_set_value) {
        int32_t data = ((value_id_and_data[1]) | ((int32_t)value_id_and_data[2] << 8) | ((int32_t)value_id_and_data[3] << 16) | ((int32_t)value_id_and_data[4] << 24));
        memcpy(eeconfig_kb.bytes + member->offset, &data, member->size);
        if (member->initialize) member->initialize(data);
    } else if (*command_id == id_custom_get_value) {
        int32_t data;
        memcpy(&data, eeconfig_kb.bytes + member->offset, member->size);
        memcpy(&value_id_and_data[1], &data, member->size);
    } else if (*command_id == id_custom_save) {
        for (int i = 0; i < member->size; i++) {
            eeprom_update_byte(EECONFIG_KB_DATABLOCK + member->offset + i, eeconfig_kb.bytes[member->offset + i]);
        }
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value_data ]
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*channel_id == id_custom_channel) {
        printf("command:%d channel:%d value_id:%d value:%d\n", *command_id, *channel_id, value_id_and_data[0], value_id_and_data[1]);
    } else {
        via_custom_value_command_user(data, length);
        return;
    }
    
    via_custom_value_command_bmp(data, length);
    process_custom_value_command(data, length);

    via_custom_value_command_user(data, length);
}
