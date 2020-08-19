#ifndef _PIN_DEFINES_H__
#define _PIN_DEFINES_H__

#ifdef NRF52810_XXAA

#define BOARDS_LED						12

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

#elif NRF52832_XXAA

#define BOARDS_LED						5

#define SPI_INSTANCE 					1

#define SPI_SCK_PIN						8
#define SPI_MOSI_PIN					21
#define SPI_MISO_PIN					6
#define SPI_SS_PIN						7

/* TWI instance ID. */
#define TWI_INSTANCE_ID 			0

#define BOARD_SDA_PIN 				18
#define BOARD_SCL_PIN 				20

#define AFE_CS_PIN 						0
#define LEADS_OFF_PIN 				28

#define ADC_ECG_CHANNEL 			NRF_SAADC_INPUT_AIN0	
#define ADC_TEMP_CHANNEL			NRF_SAADC_INPUT_AIN1
#define ADC_BATT_CHANNEL			NRF_SAADC_INPUT_AIN2
#define ADC_LEADSOFF_CHANNEL  NRF_SAADC_INPUT_AIN4

#endif

#endif
