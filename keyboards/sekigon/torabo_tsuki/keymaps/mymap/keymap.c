// Copyright 2024 ren-mntn
// Ported from Keyball44 Remap keymap (7 layers)
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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Typing mode (ported from Keyball44)
// トラックボール移動でマウスモード、キー入力でタイピングモードに自動切替
/////////////////////////////////////////////////////////////////////////////////////////////////////

#define TYPING_MODE_TIMEOUT 300
#define LEFT_CLICK_KEYCODE KC_J
#define RIGHT_CLICK_KEYCODE KC_L
#define SCROLL_CLICK_KEYCODE KC_K

static uint16_t typing_timer = 0;
static bool is_typing_mode = false;
static bool is_fixed_scroll_mode = false;
static uint16_t last_keycode = 0;
static uint16_t current_pressed_key = KC_NO;

// スクロールモード判定 (momentary DRAG_SCROLL or fixed)
static bool get_scroll_active(void) {
    return is_fixed_scroll_mode || (set_scrolling & 1);
}

// トラックボール移動があったら呼ばれる
void set_typing_false(void) {
    if (is_typing_mode) {
        is_typing_mode = false;
        if (current_pressed_key != KC_NO) {
            unregister_code16(current_pressed_key);
            current_pressed_key = KC_NO;
        }
        // 固定スクロールモードも解除
        if (is_fixed_scroll_mode) {
            is_fixed_scroll_mode = false;
            set_scrolling &= ~(1 << 0);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// LAYOUT mapping (torabo-tsuki)
//
// LAYOUT parameter order (64 keys):
//   Row 0 (number, M=unused): 3,0 3,1 3,2 3,3 3,4 3,5  8,5 8,4 8,3 8,2 8,1 8,0
//   Row 1 (top alpha):        0,0 0,1 0,2 0,3 0,4 0,5  5,5 5,4 5,3 5,2 5,1 5,0
//   Row 2 (home):             1,0 1,1 1,2 1,3 1,4 1,5  6,5 6,4 6,3 6,2 6,1 6,0
//   Row 3 (bottom+extra):     2,0 2,1 2,2 2,3 2,4 2,5 2,6  7,6 7,5 7,4 7,3 7,2 7,1 7,0
//   Row 4 (thumb):            4,0 4,1 4,2 4,3 4,4 4,5 4,6  9,6 9,5 9,4 9,3 9,2 9,1 9,0
//
// Keyball44 → torabo-tsuki mapping:
//   KB Row 0 (Tab,Q,W,E,R,T / Y,U,I,O,P,BS) → Row 1
//   KB Row 1 (GUI,A,S,D,F,G / H,BTN1,SCRL,BTN2,;,-)  → Row 2
//   KB Row 2 (Sft,Z,X,C,V,B / N,M,,,.,/,Sft) → Row 3 (col6 = extra)
//   KB Thumb → Row 4
/////////////////////////////////////////////////////////////////////////////////////////////////////

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // Layer 0: Base (QWERTY) - ported from Keyball44
    [0] = LAYOUT(
        // Row 0: number row (M has no physical keys here)
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        // Row 1: top alpha ← Keyball44 Row 0
        KC_TAB , KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   ,      KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_BSPC,
        // Row 2: home row ← Keyball44 Row 1
        KC_LGUI, KC_A   , KC_S   , KC_D   , KC_F   , KC_G   ,      KC_H   , KC_BTN1, DRAG_SCROLL, KC_BTN2, LT(6,KC_SCLN), LT(5,KC_MINS),
        // Row 3: bottom ← Keyball44 Row 2 + col6 extra
        KC_LSFT, KC_Z   , KC_X   , KC_C   , KC_V   , KC_B   , KC_NO,    KC_NO, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
        // Row 4: thumb ← Keyball44 Thumb (expanded)
        KC_NO  , KC_NO  , KC_LALT, KC_RCTL, LCTL_T(KC_LNG2), LT(2,KC_SPC), LT(4,KC_LNG1),    LT(3,KC_ESC), LT(1,KC_ENT), KC_LNG1, KC_RGUI, KC_ESC , KC_NO  , KC_NO
    ),

    // Layer 1: Editor shortcuts (Win/Alt combos)
    [1] = LAYOUT(
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        TG(6)  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      G(KC_R), KC_NO  , RCTL(KC_PGUP), RCTL(KC_PGDN), G(KC_F), S(G(KC_F)),
        KC_LGUI, A(KC_A), A(KC_S), A(KC_D), A(KC_F), KC_NO  ,      KC_BTN4, G(KC_C), G(KC_V), S(G(KC_C)), C(KC_GRV), G(KC_W),
        KC_LSFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO,    KC_NO, S(KC_8), G(KC_X), S(G(KC_V)), S(G(KC_Y)), C(S(A(KC_R))), C(A(KC_T)),
        KC_NO  , KC_NO  , KC_LALT, KC_LCTL, KC_NO  , KC_SPC , KC_NO,    KC_NO, KC_NO  , _______, _______, KC_ESC , KC_NO  , KC_NO
    ),

    // Layer 2: Symbols
    [2] = LAYOUT(
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , C(S(KC_E)), C(S(KC_R)), S(G(KC_T)),    KC_GRV , S(KC_4), S(KC_9), S(KC_0), S(KC_7), KC_BSPC,
        KC_LGUI, KC_NO  , KC_NO  , KC_NO  , G(KC_P), C(S(G(KC_E))),    S(KC_2), S(KC_1), KC_LBRC, KC_RBRC, S(KC_BSLS), KC_BSLS,
        KC_LSFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO,    KC_NO, S(KC_3), KC_QUOT, KC_COMM, KC_DOT , KC_SLSH, KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_LGUI,    KC_RGUI, G(KC_ENT), _______, _______, KC_NO  , KC_NO  , KC_NO
    ),

    // Layer 3: Neovim navigation (Ctrl combos)
    [3] = LAYOUT(
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_BSPC,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      C(S(KC_E)), C(KC_Q), C(KC_G), C(KC_GRV), C(S(KC_GRV)), KC_NO,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO,    KC_NO, C(KC_H), C(KC_J), C(KC_K), C(KC_L), KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_LALT, KC_NO,    KC_NO, KC_NO  , _______, _______, KC_NO  , KC_NO  , KC_NO
    ),

    // Layer 4: Numbers
    [4] = LAYOUT(
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      S(KC_COMM), S(KC_5), KC_EQL, S(KC_EQL), S(KC_8), KC_BSPC,
        KC_LGUI, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_1   , KC_2   , KC_3   , KC_4   , KC_5   , KC_MINS,
        KC_LSFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO,    KC_NO, KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , S(KC_SCLN),
        KC_NO  , KC_NO  , KC_NO  , KC_LCTL, KC_NO  , KC_SPC , KC_NO,    KC_NO, KC_NO  , _______, _______, KC_NO  , KC_NO  , KC_NO
    ),

    // Layer 5: Window management / Screenshots
    [5] = LAYOUT(
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      S(G(KC_4)), C(S(G(KC_4))), C(S(KC_S)), KC_NO, C(S(A(KC_P))), KC_NO,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      C(S(A(KC_H))), C(S(A(KC_J))), C(S(A(KC_K))), C(S(A(KC_L))), C(S(A(KC_SCLN))), KC_NO,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO,    KC_NO, C(S(A(KC_N))), C(KC_TAB), A(KC_TAB), KC_NO, KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO,    KC_NO, G(KC_SPC), _______, _______, KC_NO, KC_NO  , KC_NO
    ),

    // Layer 6: Gaming / Number row
    [6] = LAYOUT(
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,      KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_ESC , KC_TAB , KC_Q   , KC_W   , KC_E   , KC_R   ,      KC_Y   , KC_U   , KC_I   , KC_O   , TG(6)  , KC_DEL ,
        KC_1   , KC_RCTL, KC_A   , KC_S   , KC_D   , KC_F   ,      KC_LEFT, KC_DOWN, KC_UP  , KC_RGHT, KC_F12 , KC_HOME,
        KC_2   , KC_LSFT, KC_Z   , KC_X   , KC_C   , KC_V   , KC_NO,    KC_NO, KC_N   , KC_M   , KC_GRV , KC_LBRC, KC_RBRC, TG(6)  ,
        KC_NO  , KC_NO  , KC_LGUI, KC_LALT, KC_3   , KC_SPC , KC_4 ,    KC_SPC, LT(1,KC_ENT), _______, _______, KC_ESC, KC_NO  , KC_NO
    ),
};

// Encoder map: trackball as pseudo-encoder (left/right, up/down per layer)
const uint16_t encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [1] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [2] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [3] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [4] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [5] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
    [6] = { {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, {KC_RIGHT, KC_LEFT}, {KC_UP, KC_DOWN}, },
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Mouse / Trackball handling
/////////////////////////////////////////////////////////////////////////////////////////////////////

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

    // Typing mode: trackball moved → exit typing mode
    if (mouse_report.x != 0 || mouse_report.y != 0) {
        set_typing_false();
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
        float norm = sqrtf(mouse_report.x * mouse_report.x + mouse_report.y * mouse_report.y);
        if (norm != 0) {
            int32_t s = 0;
            for (int i = 0; i < CURVE_POINT; i++) {
                if (norm < eeconfig_kb.cursor.curve.shift_point[i] * 10 || i == CURVE_POINT - 1 || eeconfig_kb.cursor.curve.shift_point[i] == 0) {
                    float g = eeconfig_kb.cursor.curve.shift_rate[i] + s / norm;
                    divide /= (g / 100.0f);
                    break;
                }
                s = (eeconfig_kb.cursor.curve.shift_rate[i] - eeconfig_kb.cursor.curve.shift_rate[i + 1]) * eeconfig_kb.cursor.curve.shift_point[i] * 10;
            }
        }
    }

    float rad = eeconfig_kb.cursor.rotate / 180.0f * M_PI;
    float c = cosf(rad);
    float sn = sinf(rad);
    float x = (c * mouse_report.x + -sn * mouse_report.y) / divide + x_kh;
    float y = (sn * mouse_report.x + c * mouse_report.y) / divide + y_kh;

    mouse_report.x = (int)x;
    mouse_report.y = (int)y;
    x_kh = x - mouse_report.x;
    y_kh = y - mouse_report.y;

    set_scrolling &= ~(1 << 1);
    set_scrolling |= (eeconfig_kb.scroll.layer & highest_layer_bits) ? 2 : 0;

    if (set_scrolling > 0 || set_encoder_flag > 0) {
        bool snap_option = ((set_scrolling > 0) && (eeconfig_kb.scroll.options.snap))
                           || ((set_encoder_flag > 0)
                               && ((eeconfig_kb.pseudo_encoder.snap_layer & highest_layer_bits) != 0));
        bool invert_option = ((set_scrolling > 0) && (eeconfig_kb.scroll.options.invert));

        if (mouse_report.x == 0 && mouse_report.y == 0) {
            mouse_report.h = 0;
            mouse_report.v = 0;
            return mouse_report;
        } else if (snap_option) {
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
                action_exec(mouse_report.h > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 0, true)
                                               : MAKE_ENCODER_CCW_EVENT(biton(set_encoder_flag) * 2 + 0, true));
                action_exec(mouse_report.h > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 0, false)
                                               : MAKE_ENCODER_CCW_EVENT(biton(set_encoder_flag) * 2 + 0, false));
            }
            if (mouse_report.v != 0) {
                action_exec(mouse_report.v > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 1, true)
                                               : MAKE_ENCODER_CCW_EVENT(biton(set_encoder_flag) * 2 + 1, true));
                action_exec(mouse_report.v > 0 ? MAKE_ENCODER_CW_EVENT(biton(set_encoder_flag) * 2 + 1, false)
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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// process_record_user: Typing mode + DRAG_SCROLL + encoder
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Encoder keys (not part of typing/scroll logic)
    if (keycode >= TRACKBALL_AS_ENCODER1 && keycode <= TRACKBALL_AS_ENCODER2) {
        set_encoder &= ~(1 << (keycode - TRACKBALL_AS_ENCODER1));
        set_encoder |= record->event.pressed ? (1 << (keycode - TRACKBALL_AS_ENCODER1)) : 0;
        return false;
    }

    // 早期リターン: typing/scroll logicで使わないキーはスルー
    if (!((keycode >= KC_A && keycode <= KC_Z) || keycode == KC_BTN1 || keycode == KC_BTN2 || keycode == DRAG_SCROLL ||
          keycode == KC_LNG1 || keycode == KC_LNG2 || IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode))) {
        return true;
    }

    // マウスクリック（BTN1, BTN2, DRAG_SCROLL）の処理
    if (keycode == KC_BTN1 || keycode == KC_BTN2 || keycode == DRAG_SCROLL) {
        // スクロールモード時のBTN1はピンチイン/アウト用のCmd+左クリック
        if (keycode == KC_BTN1) {
            if (get_scroll_active()) {
                if (record->event.pressed) {
                    register_code(KC_LGUI);
                } else {
                    unregister_code(KC_LGUI);
                }
                return false;
            }
        }

        if (record->event.pressed) {
            // タイピングモード中: BTN1→J, BTN2→L, DRAG_SCROLL→K
            if (is_typing_mode) {
                uint16_t target_keycode = (keycode == KC_BTN1) ? KC_J :
                                         (keycode == KC_BTN2) ? KC_L :
                                         KC_K; // DRAG_SCROLL
                if (current_pressed_key != KC_NO) {
                    unregister_code16(current_pressed_key);
                }
                current_pressed_key = target_keycode;
                register_code16(target_keycode);
                return false;
            } else {
                // 通常モード: マウスクリック動作 + 自動補完用の設定
                if (keycode == DRAG_SCROLL) {
                    // DRAG_SCROLL: スクロールモードON
                    set_scrolling |= (1 << 0);
                }
                uint16_t target_keycode = (keycode == KC_BTN1) ? KC_J :
                                         (keycode == KC_BTN2) ? KC_L :
                                         KC_K;
                last_keycode = target_keycode;
                typing_timer = timer_read();
                if (keycode == DRAG_SCROLL) return false; // DRAG_SCROLLはクリック送信しない
                return true;
            }
        } else {
            // キーリリース
            if (is_typing_mode && current_pressed_key != KC_NO) {
                unregister_code16(current_pressed_key);
                current_pressed_key = KC_NO;
                return false;
            }
            if (keycode == DRAG_SCROLL) {
                // 固定スクロールでなければ解除
                if (!is_fixed_scroll_mode) {
                    set_scrolling &= ~(1 << 0);
                }
                return false;
            }
            return true;
        }
    }

    // 言語切り替えキー → タイピングモードにする
    if (!is_typing_mode) {
        bool is_lng_key = false;
        if (keycode == KC_LNG1 || keycode == KC_LNG2) {
            is_lng_key = true;
        } else if (IS_QK_MOD_TAP(keycode)) {
            uint16_t tap_kc = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
            if (tap_kc == KC_LNG1 || tap_kc == KC_LNG2) is_lng_key = true;
        } else if (IS_QK_LAYER_TAP(keycode)) {
            uint16_t tap_kc = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
            if (tap_kc == KC_LNG1 || tap_kc == KC_LNG2) is_lng_key = true;
        }
        if (is_lng_key) {
            is_typing_mode = true;
            return true;
        }
    }

    // スクロールモード中の処理
    if (get_scroll_active()) {
        // 固定スクロール中にクリック以外のキー → スクロール解除
        if (is_fixed_scroll_mode && record->event.pressed) {
            is_fixed_scroll_mode = false;
            set_scrolling &= ~(1 << 0);
            return false;
        }
        // DRAG_SCROLL固定時はリリースを無視（上で処理済みだが念のため）
        if (keycode == DRAG_SCROLL) {
            if (!record->event.pressed && is_fixed_scroll_mode)
                return false;
            return false;
        }
        // スクロール中にBTN2 → 固定スクロールモード
        if (keycode == KC_BTN2) {
            if (record->event.pressed) {
                is_fixed_scroll_mode = true;
                set_scrolling |= (1 << 0);
            }
            return false;
        }
    }

    // キーリリースはスルー
    if (!record->event.pressed) return true;

    // A-Zキー入力 → タイピングモードにする
    if (!is_typing_mode) is_typing_mode = true;

    // 母音自動補完 (クリック後300ms以内に母音 → 子音を自動挿入)
    if (typing_timer != 0 && timer_elapsed(typing_timer) <= TYPING_MODE_TIMEOUT) {
        bool is_vowel = (keycode == KC_A || keycode == KC_I || keycode == KC_U || keycode == KC_E || keycode == KC_O);
        bool is_consonant = (last_keycode == KC_J || last_keycode == KC_K || last_keycode == KC_L);
        if (is_vowel && is_consonant) {
            typing_timer = 0;
            tap_code16(last_keycode);
        } else {
            typing_timer = 0;
        }
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
