#include "data_storage.h"

#include "data_fds.h"

#include "nrf_drv_spi.h"
#include "nand_spi_flash.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "custom_board.h"

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
static volatile bool spi_xfer_done = true;

static void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_context)
{
    spi_xfer_done = true;
}

static void nand_spi_init()
{

    nrf_drv_spi_config_t spi_config = {
        .sck_pin = SPI_SCK_PIN,
        .mosi_pin = SPI_MOSI_PIN,
        .miso_pin = SPI_MISO_PIN,
        .ss_pin = SPI_SS_PIN,
        .irq_priority = SPI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .orc = 0x00,
        .frequency = NRF_DRV_SPI_FREQ_8M,
        .mode = NRF_DRV_SPI_MODE_0,
        .bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
    };

    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
}

int nand_spi_transfer(uint8_t *buffer, uint16_t tx_len, uint16_t rx_len)
{

    spi_xfer_done = false;
	
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, buffer, tx_len, buffer, tx_len + rx_len));

    while (!spi_xfer_done) nrf_pwr_mgmt_run();
		
    return NSF_ERR_OK;
}

void nand_spi_delayus(uint32_t delay)
{

    nrf_delay_us(delay);
}

void nand_flash_prepare(void)
{
    int errid;
		ret_code_t ret;

    nand_spi_init();
    nand_spi_flash_config_t config = {nand_spi_transfer, nand_spi_delayus};

    errid = nand_spi_flash_init(&config);
    NRF_LOG_INFO("nand_spi_flash_init:%s", nand_spi_flash_str_error(errid));
    errid = nand_spi_flash_reset_unlock();
    NRF_LOG_INFO("nand_spi_flash_reset_unlock:%s", nand_spi_flash_str_error(errid));
		
}


