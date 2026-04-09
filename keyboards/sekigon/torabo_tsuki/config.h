// Copyright 2024 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#ifndef __ASSEMBLER__

#define BOOTMAGIC_LITE_ROW 0
#define BOOTMAGIC_LITE_COLUMN 1

#define EECONFIG_KB_DATA_SIZE 32
#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DELAY get_auto_mouse_delay()
#define AUTO_MOUSE_THRESHOLD get_auto_mouse_threshold()
unsigned short get_auto_mouse_delay(void);
unsigned char  get_auto_mouse_threshold(void);

#define CONFIG_SCK_PIN 9
#define CONFIG_SS_PIN 8
#define CONFIG_HALF_DUPLEX_PIN 7
#define CONFIG_SPI_FREQ SPI_FREQ_2M
#define CONFIG_SPI_MODE 3

#define MATRIX_ROWS_DEFAULT 10
#define MATRIX_COLS_DEFAULT 7
#define MATRIX_SCAN_TIME_MS 15
#define ACTION_DEBUG

#define BMP_DEFAULT_MODE 1
#define THIS_DEVICE_ROWS (MATRIX_ROWS_DEFAULT / 2)
#define THIS_DEVICE_COLS MATRIX_COLS_DEFAULT
#define BMP_DIODE_DIRECTION MATRIX_74HC164COL
#define BMP_STARTUP_ADVERTISE 1
#define BMP_USE_DEFAULT_VIAL_CONFIG
#define BMP_INDICATOR_INVERT 1
#define BMP_VIAL_MODE_DEFAULT true
#define BMP_DEBOUNCE 2

#define CONFIG_RESERVED {0, 2, 0, 0, 0, 0, 0, 0}

#    include "microshell/util/mscmd.h"
// #    define USER_DEFINED_MSCMD

#endif