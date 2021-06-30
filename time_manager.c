/**
 * @file    mc3672.c
 * @author  Simpreative
 * @date    June 2021
 * @brief   Percision (millseconds) time synchronization management
 * @see     https://github.com/imcort-nrf-drivers
 */
#include "time_manager.h"
#include "app_timer.h"

int64_t millis = 0;           //Power On to now period
int64_t power_on_time = 0;    //Power On time

bool time_is_calibrated = false;

APP_TIMER_DEF(millis_timer);

static void m_millis_timer_handler(void *p_context)
{
    millis++;
}

void time_manager_begin(void)
{
    ret_code_t err_code;
	err_code = app_timer_create(&millis_timer, APP_TIMER_MODE_REPEATED, m_millis_timer_handler);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_start(millis_timer, APP_TIMER_TICKS(1), NULL);
    APP_ERROR_CHECK(err_code);
    
}

void time_calibrate(uint32_t now_time_in_seconds)
{
    
    power_on_time = ((int64_t)now_time_in_seconds * 1000) - millis;
    time_is_calibrated = true;
    
}

void time_ms_uint8_arr_get(uint8_t* time_arr) //length = 8
{
    
    *(int64_t*)time_arr = (millis + power_on_time);

}

uint32_t time_s_get(void)
{
    
    return ((millis + power_on_time) / 1000);

}
