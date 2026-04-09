// Copyright 2024 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include QMK_KEYBOARD_H
#include <math.h>
#include "bmp.h"
#include "bmp_custom_keycodes.h"
#include "quantum.h"

uint8_t set_scrolling = 0;
uint8_t set_encoder = 0;

enum {
    DRAG_SCROLL = BMP_SAFE_RANGE,
    TRACKBALL_AS_ENCODER1,
    TRACKBALL_AS_ENCODER2,
};

// Disable BMP dynamic matrix size
#undef MATRIX_ROWS
#define MATRIX_ROWS MATRIX_ROWS_DEFAULT
#undef MATRIX_COLS
#define MATRIX_COLS MATRIX_COLS_DEFAULT

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_ESC, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_BSPC,
        KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC,
        KC_CAPS, MT(MOD_LCTL, KC_A), KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT,
        KC_LSFT, MT(MOD_LSFT, KC_Z), KC_X, KC_C, KC_V, KC_B, LCTL(KC_C), LCTL(KC_V), KC_N, KC_M, KC_COMM, KC_DOT, MT(MOD_RSFT, KC_SLSH), KC_RSFT,
        KC_LCTL, KC_LCTL, KC_LGUI, KC_LALT, KC_BSPC, LT(2, KC_SPC), MT(MOD_LSFT, KC_LNG1), MT(MOD_RSFT, KC_LNG2), LT(3, KC_ENT), KC_BSPC, KC_RALT, KC_RGUI, KC_RCTL, KC_RCTL
    ),
    [1] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, KC_MS_BTN2, KC_MS_BTN1, _______, _______, KC_MS_BTN1, KC_MS_BTN2, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______
    ),
    [2] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, S(KC_1), S(KC_2), S(KC_3), S(KC_4), S(KC_5), S(KC_6), S(KC_7), S(KC_8), S(KC_9), S(KC_0), _______,
        _______, KC_1, KC_2, KC_3, KC_4, KC_5, KC_LCBR, KC_MINS, KC_EQL, KC_RCBR, KC_COLN, _______,
        _______, KC_6, KC_7, KC_8, KC_9, KC_0, _______, _______, KC_UNDS, KC_PLUS, KC_LBRC, KC_RBRC, KC_BSLS, _______,
        _______, _______, _______, _______, _______, _______, _______, BATT_LV, KC_TAB, KC_DEL, _______, _______, _______, _______
    ),
    [3] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5,  KC_HOME, KC_PGDN, KC_PGUP, KC_END, KC_ESC, _______,
        _______, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_LEFT, KC_DOWN, KC_UP, KC_RIGHT, KC_TAB, _______,
        _______, KC_F11, KC_F12, SEL_USB, SEL_BLE, AD_WO_L, _______, _______, KC_QUOT, KC_DQT, KC_GRV, KC_TILD, KC_PIPE, _______,
        _______, _______, _______, _______, KC_DEL, KC_TAB, BATT_LV, _______, _______, _______, _______, _______, _______, _______
    ),
};

const uint16_t encoder_map[4][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [1] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [2] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [3] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
};

bool is_mouse_record_user(uint16_t keycode, keyrecord_t* record) {
    switch(keycode) {
        case KC_NO:
        case KC_LEFT_CTRL...KC_RIGHT_GUI:
        case DRAG_SCROLL:
        case TRACKBALL_AS_ENCODER1 ... TRACKBALL_AS_ENCODER2:
            return true;
        default:
            return false;
    }
    return false;
}

#define SNAP_BUF_LEN 5
#define SNAP_TIME_OUT 500

typedef enum {
    SNAP_NONE,
    SNAP_X,
    SNAP_Y,
} snap_state_t;

snap_state_t update_snap(snap_state_t current_snap, uint8_t snap_history) {
    uint8_t x_cnt = 0;
    for (int i = 0; i < SNAP_BUF_LEN; i++) {
        if (snap_history & (1 << i)) x_cnt++;
    }

    if (current_snap == SNAP_X && x_cnt <= SNAP_BUF_LEN / 2 && (snap_history & 0x03) == 0) {
        return SNAP_Y;
    } else if (current_snap == SNAP_Y && x_cnt > SNAP_BUF_LEN / 2 && (snap_history & 0x03) == 3) {
        return SNAP_X;
    } else if (current_snap == SNAP_NONE) {
        return (x_cnt <= SNAP_BUF_LEN / 2) ? SNAP_Y : SNAP_X;
    }

    return current_snap;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    static bool         snap_start;
    static int8_t       snap_buf_cnt;
    static uint8_t      snap_history;
    static snap_state_t snap_state;
    static float        x_kh, y_kh;
    static uint32_t     last_scroll_time;

    if (!is_pointing_device_active()) {
        return mouse_report;
    }

    uint32_t highest_layer_bits = (1 << get_highest_layer(layer_state));
    uint8_t  set_encoder_layer  = (eeconfig_kb.pseudo_encoder.layer & highest_layer_bits) ? (1 << 0) : 0;
    uint8_t  set_encoder_flag   = set_encoder_layer | set_encoder;

    float divide = 1;
    if (set_scrolling > 0 && eeconfig_kb.scroll.divide > 0) {
        divide *= eeconfig_kb.scroll.divide;
    }

    if (set_encoder_flag > 0 && eeconfig_kb.pseudo_encoder.divide > 0) {
        divide *= eeconfig_kb.pseudo_encoder.divide;
    }

    if ((eeconfig_kb.cursor.fine_layer & highest_layer_bits) && eeconfig_kb.cursor.fine_div > 0) {
        divide *= eeconfig_kb.cursor.fine_div;
    }

    if ((eeconfig_kb.cursor.rough_layer & highest_layer_bits) && eeconfig_kb.cursor.rough_mul > 0) {
        divide /= eeconfig_kb.cursor.rough_mul;
    }

    if (eeconfig_kb.cursor.curve.shift_point[0] != 0) {
        // scale mouse report by piecewise linear function
        float norm = sqrtf(mouse_report.x * mouse_report.x + mouse_report.y * mouse_report.y);
        if (norm != 0) {
            int32_t s = 0;
            for (int i = 0; i < CURVE_POINT; i++) {
                if (norm < eeconfig_kb.cursor.curve.shift_point[i] * 10 || i == CURVE_POINT - 1 || eeconfig_kb.cursor.curve.shift_point[i] == 0) {
                    // y = a[n]|x| + (a[n-1]-a[n])x[n-1]
                    // g = y/|x| = a[n] + (a[n-1]-a[n])x[n-1]/|x|
                    float g = eeconfig_kb.cursor.curve.shift_rate[i] + s / norm;
                    if (debug_config.mouse) {
                        printf("\tshift %d\n", i + 1);
                        // printf("\tgain: %d %%, s;%ld\n", (int)g, s);
                    }
                    divide /= (g / 100.0f);

                    break;
                }
                s = (eeconfig_kb.cursor.curve.shift_rate[i] - eeconfig_kb.cursor.curve.shift_rate[i + 1]) * eeconfig_kb.cursor.curve.shift_point[i] * 10;
            }
        }
    }

    float rad = eeconfig_kb.cursor.rotate / 180.0f * M_PI;
    float c = cosf(rad);
    float s = sinf(rad);
    float x   = (c * mouse_report.x + -s * mouse_report.y) / divide + x_kh;
    float y   = (s * mouse_report.x + c * mouse_report.y) / divide + y_kh;

    mouse_report.x = (int)x;
    mouse_report.y = (int)y;
    x_kh           = x - mouse_report.x;
    y_kh           = y - mouse_report.y;

    set_scrolling &= ~(1 << 1);
    set_scrolling |= (eeconfig_kb.scroll.layer & highest_layer_bits) ? 2 : 0;

    if (set_scrolling > 0 || set_encoder_flag > 0) {
        bool snap_option = ((set_scrolling > 0) && (eeconfig_kb.scroll.options.snap)) //
                           || ((set_encoder_flag > 0)                                 //
                               && ((eeconfig_kb.pseudo_encoder.snap_layer & highest_layer_bits) != 0));
        bool invert_option = ((set_scrolling > 0) && (eeconfig_kb.scroll.options.invert)); //

        if (mouse_report.x == 0 && mouse_report.y == 0) {
            mouse_report.h = 0;
            mouse_report.v = 0;
            return mouse_report;
        }
        else if (snap_option) {
            if (!snap_start || timer_elapsed32(last_scroll_time) > SNAP_TIME_OUT) {
                snap_start   = true;
                snap_buf_cnt = 0;
                snap_history = 0;
                snap_state   = SNAP_NONE;
            }
            last_scroll_time = timer_read32();

            snap_history <<= 1;
            if (snap_state == SNAP_X) {
                snap_history |= abs(mouse_report.y) > abs(mouse_report.x) * 4 ? 0 : 1;
            } else {
                snap_history |= abs(mouse_report.x) > abs(mouse_report.y) * 4 ? 1 : 0;
            }
            snap_buf_cnt += snap_buf_cnt < SNAP_BUF_LEN ? 1 : 0;
            snap_history &= ((1 << SNAP_BUF_LEN) - 1);
            if (snap_buf_cnt == SNAP_BUF_LEN) {
                snap_state = update_snap(snap_state, snap_history);
                if (snap_state == SNAP_X) {
                    mouse_report.h = (mouse_report.x > 127) ? 127 : ((mouse_report.x < -127) ? -127 : mouse_report.x);
                } else {
                    mouse_report.v = (mouse_report.y > 127) ? -127 : ((mouse_report.y < -127) ? 127 : -mouse_report.y);
                }
            }
        } else {
            mouse_report.h = (mouse_report.x > 127) ? 127 : ((mouse_report.x < -127) ? -127 : mouse_report.x);
            mouse_report.v = (mouse_report.y > 127) ? -127 : ((mouse_report.y < -127) ? 127 : -mouse_report.y);
        }

        if (invert_option) {
            mouse_report.h *= -1;
            mouse_report.v *= -1;
        }

        mouse_report.x = 0;
        mouse_report.y = 0;

        if (set_encoder_flag > 0) {
            if (mouse_report.h != 0) {
                action_exec(mouse_report.h > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 0, true) //
                                               : MAKE_ENCODER_CCW_EVENT(biton(set_encoder_flag) * 2 + 0, true));
                action_exec(mouse_report.h > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 0, false) //
                                               : MAKE_ENCODER_CCW_EVENT(biton(set_encoder_flag) * 2 + 0, false));
            }
            if (mouse_report.v != 0) {
                action_exec(mouse_report.v > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 1, true) //
                                               : MAKE_ENCODER_CCW_EVENT(biton(set_encoder_flag) * 2 + 1, true));
                action_exec(mouse_report.v > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 1, false) //
                                               : MAKE_ENCODER_CCW_EVENT(biton(set_encoder_flag) * 2 + 1, false));
            }
        }

        if (set_scrolling == 0) {
            mouse_report.h = 0;
            mouse_report.v = 0;
        }

    } else {
        snap_start = false;
    }

    return mouse_report;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case DRAG_SCROLL:
            set_scrolling &= ~(1 << 0);
            set_scrolling |= record->event.pressed ? 1 : 0;
            break;

        case TRACKBALL_AS_ENCODER1 ... TRACKBALL_AS_ENCODER2:
            set_encoder &= ~(1 << (keycode - TRACKBALL_AS_ENCODER1));
            set_encoder |= record->event.pressed ? (1 << (keycode - TRACKBALL_AS_ENCODER1)) : 0;
            break;
    }

    return true;
}

// perform as ignore mod tap interrupt
extern bool __real_get_hold_on_other_key_press_vial(uint16_t keycode, keyrecord_t *record);
bool __wrap_get_hold_on_other_key_press(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case QK_MOD_TAP ... QK_MOD_TAP_MAX:
            return __real_get_hold_on_other_key_press_vial(keycode, record);
        default:
            return true;
    }
}
