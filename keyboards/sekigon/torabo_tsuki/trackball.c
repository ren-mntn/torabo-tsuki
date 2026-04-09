// Copyright 2024 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "trackball.h"

#include "gpio.h"
#include "spi.h"
#include "debug.h"
#include "wait.h"

enum {
    REG_ADDR_PROD_ID = 0x00,
    REG_ADDR_REV_ID = 0x01,
    REG_ADDR_MOTION = 0x02,
    REG_ADDR_X_L = 0x03,
    REG_ADDR_Y_L = 0x04,
    REG_ADDR_XY_H = 0x05,
    REG_ADDR_PERFORMANCE = 0x11,
    REG_ADDR_BURST_READ = 0x12,
    REG_ADDR_RUN_DOWNSHIFT = 0x1b,
    REG_ADDR_REST1_RATE = 0x1c,
    REG_ADDR_REST1_DOWNSHIFT = 0x1d,
    REG_ADDR_OBSERVATION1 = 0x2d,
    REG_ADDR_WAKEUP = 0x3a,
    REG_ADDR_SHUTDOWN = 0x3b,
    REG_ADDR_SPI_CLK_ON = 0x41,
    REG_ADDR_PAGE0 = 0x7f,
    REG_ADDR_WRITE_FLAG = 0x80,
    REG_ADDR_RES_STEP = 0x85,
};

enum {
    REG_VALUE_SCLK_ENABLE = 0xba,
    REG_VALUE_SCLK_DISABLE = 0xb5,
    REG_VALUE_PAGE1_SWITCH = 0xff,
    REG_VALUE_PAGE0_SWITCH = 0x00,
};

static uint8_t cs;

static void write_reg(uint8_t addr, uint8_t val)
{
    gpio_write_pin_low(cs);

    uint8_t dat[2];
    dat[0] = REG_ADDR_SPI_CLK_ON | REG_ADDR_WRITE_FLAG;
    dat[1] = REG_VALUE_SCLK_ENABLE;
    spim_start(dat, 2, NULL, 0, 0);

    dat[0] = addr | REG_ADDR_WRITE_FLAG;
    dat[1] = val;
    spim_start(dat, 2, NULL, 0, 0);

    dat[0] = REG_ADDR_SPI_CLK_ON | REG_ADDR_WRITE_FLAG;
    dat[1] = REG_VALUE_SCLK_DISABLE;
    spim_start(dat, 2, NULL, 0, 0);

    gpio_write_pin_high(cs);
}

static uint8_t read_reg(uint8_t addr)
{
    gpio_write_pin_low(cs);

    uint8_t dat = addr;
    spim_start(&dat, 1, NULL, 0, 0);
    spim_start(NULL, 0, &dat, 1, 0);

    gpio_write_pin_high(cs);

    return dat;
}

int trackball_init(uint8_t cs_pin) {
    int res = 0;

    cs = cs_pin;
    spim_init();

    uint8_t retry = 10;
    do {
        gpio_write_pin_low(cs);
        wait_us(150);
        gpio_write_pin_high(cs);
        wait_us(150);

        trackball_wakeup();

        write_reg(REG_ADDR_OBSERVATION1, 0x00);
        wait_us(10 * 1000);

        uint8_t obs = read_reg(REG_ADDR_OBSERVATION1);
        if ((obs & 0x7) != 0x7) {
            // fail
            res = 1;
        }
    } while(res == 1 && --retry > 0);

    read_reg(REG_ADDR_MOTION);
    read_reg(REG_ADDR_X_L);
    read_reg(REG_ADDR_Y_L);
    read_reg(REG_ADDR_XY_H);

    res = read_reg(REG_ADDR_PROD_ID) == 0x3e ? 0 : 1;

    // 8ms, 8ms, 8ms
    write_reg(REG_ADDR_PERFORMANCE, 0);

    return res;
}

int trackball_wakeup(void) {
    write_reg(REG_ADDR_WAKEUP, 0x96);
    return 0;
}

void trackball_shutdown(void) {
    write_reg(REG_ADDR_SHUTDOWN, 0xe7);
}

trackball_data_t trackball_get(void) {
    gpio_write_pin_low(cs);

    uint8_t dat[4] = {REG_ADDR_BURST_READ};
    spim_start(dat, 1, NULL, 0, 0);
    spim_start(NULL, 0, dat, 4, 0);

    gpio_write_pin_high(cs);

    trackball_data_t res = {
        .stat = dat[0],
        .x    = (int16_t)((dat[1] << 4) | ((dat[3] & 0xf0) << 8)) >> 4,
        .y    = (int16_t)((dat[2] << 4) | ((dat[3] & 0x0f) << 12)) >> 4,
    };

    return res;
}

static uint16_t cpi;

void trackball_set_cpi(uint16_t cpi_) {
    cpi_ = (cpi_ / 200) * 200;
    cpi_ = MIN(cpi_, 3200);
    cpi_ = MAX(cpi_, 200);

    // set cpi
    write_reg(REG_ADDR_PAGE0, REG_VALUE_PAGE1_SWITCH);
    write_reg(REG_ADDR_RES_STEP, cpi_ / 200);
    write_reg(REG_ADDR_PAGE0, REG_VALUE_PAGE0_SWITCH);

    cpi = cpi_;
}

uint16_t trackball_get_cpi(void) {
    return cpi;
}