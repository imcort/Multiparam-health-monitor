#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

#include "transfer_handler.h"

void time_manager_begin(void);
void time_calibrate(uint32_t now_time_in_seconds);
void time_ms_uint8_arr_get(uint8_t* time_arr);
uint32_t time_s_get(void);

#endif
