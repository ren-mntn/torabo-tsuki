
#pragma once

#include <stdint.h>

typedef struct {
    uint8_t stat;
    int16_t x;
    int16_t y;
} trackball_data_t;

int trackball_init(uint8_t cs_pin);
int trackball_enable(void);
trackball_data_t trackball_get(void);
uint8_t trackball_get_status(void);
void trackball_set_cpi(uint16_t cpi);
uint16_t trackball_get_cpi(void);

int trackball_sleep(void);
int trackball_wakeup(void);
void trackball_shutdown(void);
