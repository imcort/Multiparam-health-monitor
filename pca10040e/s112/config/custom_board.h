#ifndef PCA10040_H
#define PCA10040_H

#include "nrf_gpio.h"

// LEDs definitions for PCA10040
#define LEDS_NUMBER    1

#define LED_START      12
#define LED_1          12
//#define LED_2          18
//#define LED_3          19
//#define LED_4          20
#define LED_STOP       2

#define LEDS_ACTIVE_STATE 1

#define LEDS_INV_MASK  LEDS_MASK

#define LEDS_LIST { LED_1 }

#define BSP_LED_0      LED_1
//#define BSP_LED_1      LED_2
//#define BSP_LED_2      LED_3
//#define BSP_LED_3      LED_4

#define BUTTONS_NUMBER 1

#define BUTTON_START   24
#define BUTTON_1       24
//#define BUTTON_2       14
//#define BUTTON_3       15
//#define BUTTON_4       16
#define BUTTON_STOP    24
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1 }

#define BSP_BUTTON_0   BUTTON_1

//#define BOARDS_LED						5

#define SPI_INSTANCE 					0

#define SPI_SCK_PIN						20
#define SPI_MOSI_PIN					21
#define SPI_MISO_PIN					8
#define SPI_SS_PIN						5

/* TWI instance ID. */
#define TWI_INSTANCE_ID 			0

#define BOARD_SDA_PIN 				15
#define BOARD_SCL_PIN 				16

#define AFE_CS_PIN 						0
#define LEADS_OFF_PIN 				14

#define ADC_ECG_CHANNEL 			NRF_SAADC_INPUT_AIN2	
#define ADC_TEMP_CHANNEL			NRF_SAADC_INPUT_AIN1
#define ADC_BATT_CHANNEL
#define ADC_LEADSOFF_CHANNEL

#endif // PCA10040_H
