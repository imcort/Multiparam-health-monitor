#ifndef _DATA_ACQUIRE_H__
#define _DATA_ACQUIRE_H__

#include <stdbool.h>

void saadc_init(void);
void timers_create(void);
void timers_start(void);
void ble_rt_send(void);
void nand_flash_data_write(void);
void nand_flash_data_read(void);

#endif
