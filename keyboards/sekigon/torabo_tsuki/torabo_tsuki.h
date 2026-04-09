#pragma once

#include "quantum.h"
#include "eeconfig_kb.h"

#define IO_RESET 5
#define TB_POW 14
#define SR_POW 1
#define SR_DATA 12
#define SR_CLK 11
#define CS_PIN_TB0 CONFIG_SS_PIN
#define TB_MOTION 10
#define BAT_IN 3

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t scroll;
    uint8_t surface;
    uint8_t motion_flag;
} trackball_info_t;

typedef enum {
    TRACKBALL_NONE,
    TRACKBALL_ADNS,
    TRACKBALL_BTO,
} trackball_type_t;

const trackball_info_t* get_trackball_info(void);
extern trackball_type_t trackball_init_flag;
void                    override_mouse_report(report_mouse_t report);
bool                    is_pointing_device_active(void);

bmp_api_ble_conn_param_t get_central_conn_param(uint8_t mode);
bmp_api_ble_conn_param_t get_periph_conn_param(uint8_t mode);