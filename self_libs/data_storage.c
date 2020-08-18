#include "data_storage.h"

#include "fds.h"
#include "nrf_drv_spi.h"
#include "nand_spi_flash.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "pin_defines.h"

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
static volatile bool spi_xfer_done = true;

static void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_context)
{
    spi_xfer_done = true;
}

#define FLASH_STATUS_FILE_ID (0x0004)
#define FLASH_OFFSET_KEY     (0x0001)
#define FLASH_READ_KEY       (0x0002)
#define FLASH_BADBLOCK_KEY   (0x0003)

static bool volatile m_fds_initialized;
static bool volatile m_fds_writed;
static bool volatile m_fds_updated;
static bool volatile m_fds_gc;

fds_find_token_t tok = {0};

fds_record_desc_t flash_offset_desc = {0};
fds_record_desc_t flash_read_desc = {0};
fds_record_desc_t flash_badblock_desc = {0};

////////////////////////////////////////////FDS

nand_flash_addr_t flash_offset = {

    .column = 0,
    .page = 0,
    .block = 0

};

nand_flash_addr_t flash_read = {

    .column = 0,
    .page = 0,
    .block = 0

};

nand_flash_badblocks_t flash_badblocks = {
		
		.bad_block_num = 0

};

fds_record_t const m_flash_offset_record = 
{
	.file_id = FLASH_STATUS_FILE_ID,
	.key = FLASH_OFFSET_KEY,
	.data.p_data = &flash_offset,
	.data.length_words = 2
};

fds_record_t const m_flash_read_record = 
{
	.file_id = FLASH_STATUS_FILE_ID,
	.key = FLASH_READ_KEY,
	.data.p_data = &flash_read,
	.data.length_words = 2
};

fds_record_t const m_flash_bad_block_record = 
{
	.file_id = FLASH_STATUS_FILE_ID,
	.key = FLASH_BADBLOCK_KEY,
	.data.p_data = &flash_badblocks,
	.data.length_words = 22
};

static void fds_evt_handler(fds_evt_t const *p_evt)
{
    //NRF_LOG_INFO("Event: %d received (%d)", p_evt->id, p_evt->result);

    switch (p_evt->id)
    {
    case FDS_EVT_INIT:
        if (p_evt->result == NRF_SUCCESS)
        {
            m_fds_initialized = true;
        }
        break;

    case FDS_EVT_WRITE:
    {
        if (p_evt->result == NRF_SUCCESS)
        {
						m_fds_writed = true;
        }
    }
    break;
		
		case FDS_EVT_UPDATE:
    {
        if (p_evt->result == NRF_SUCCESS)
        {
						m_fds_updated = true;
        }
    }
    break;
		
		case FDS_EVT_GC:
    {
        if (p_evt->result == NRF_SUCCESS)
        {
						m_fds_gc = true;
        }
    }
    break;

    default:
        break;
    }
}

void fds_prepare(void)
{

    ret_code_t err_code;

    (void)fds_register(fds_evt_handler);

    err_code = fds_init();
    APP_ERROR_CHECK(err_code);

    while (!m_fds_initialized) nrf_pwr_mgmt_run();
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

void fds_gc_process(void){
	
		//gc every pages full
		m_fds_gc = false;
		ret_code_t ret = fds_gc();
		APP_ERROR_CHECK(ret);
		while(!m_fds_gc) {
					
					//nrf_pwr_mgmt_run();
			
		}
}

void nand_fds_open(void* desc, void* dest, size_t size)
{
		ret_code_t ret;
	
		fds_flash_record_t temp = {0};
		ret = fds_record_open(desc, &temp);
		APP_ERROR_CHECK(ret);
		memcpy(dest, temp.p_data, size);
		ret = fds_record_close(desc);
		APP_ERROR_CHECK(ret);

}

void nand_fds_write(void* desc, const void* src)
{
		ret_code_t ret;
	
		m_fds_writed = false;
		ret = fds_record_write(desc, src);
		APP_ERROR_CHECK(ret);
		while(!m_fds_writed) nrf_pwr_mgmt_run();
		
}

void nand_fds_update(void* desc, const void* src)
{
		ret_code_t ret;
	
		m_fds_updated = false;
		ret = fds_record_update(desc, src);
		APP_ERROR_CHECK(ret);
		while(!m_fds_updated) nrf_pwr_mgmt_run();
		
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
		
		fds_gc_process();
		
		//Preparing flash offset
		memset(&tok, 0x00, sizeof(fds_find_token_t));
		ret = fds_record_find(FLASH_STATUS_FILE_ID, FLASH_OFFSET_KEY, &flash_offset_desc, &tok);
		
		if(ret == NRF_SUCCESS)
		{
		
			NRF_LOG_INFO("FDS found flash offset, reading it");
			nand_fds_open(&flash_offset_desc, &flash_offset, sizeof(flash_offset));
			
		} else {
			
			NRF_LOG_INFO("FDS flash offset not found. writing");
			nand_fds_write(&flash_offset_desc, &m_flash_offset_record);
			NRF_LOG_INFO("FDS flash offset writed.");
		
		}
		
		//Preparing flash read
		memset(&tok, 0x00, sizeof(fds_find_token_t));
		ret = fds_record_find(FLASH_STATUS_FILE_ID, FLASH_READ_KEY, &flash_read_desc, &tok);
		
		if(ret == NRF_SUCCESS)
		{
		
			NRF_LOG_INFO("FDS found flash read, reading it");
			nand_fds_open(&flash_read_desc, &flash_read, sizeof(flash_read));
			
		} else {
			
			NRF_LOG_INFO("FDS flash read not found. writing");
			nand_fds_write(&flash_read_desc, &m_flash_read_record);
			NRF_LOG_INFO("FDS flash read writed.");
		
		}
		
		//Preparing flash badblock
		memset(&tok, 0x00, sizeof(fds_find_token_t));
		ret = fds_record_find(FLASH_STATUS_FILE_ID, FLASH_BADBLOCK_KEY, &flash_badblock_desc, &tok);
		
		if(ret == NRF_SUCCESS)
		{
		
			NRF_LOG_INFO("FDS found flash badblock, reading it");
			nand_fds_open(&flash_badblock_desc, &flash_badblocks, sizeof(flash_badblocks));
			
		} else {
			
			NRF_LOG_INFO("FDS flash badblock not found. writing");
			nand_fds_write(&flash_badblock_desc, &m_flash_bad_block_record);
			NRF_LOG_INFO("FDS flash badblock writed.");
		
		}
		
}

bool is_bad_block_existed(uint16_t bbnum)
{

    for (int i = 0; i < flash_badblocks.bad_block_num; i++)
    {
			
        if (flash_badblocks.bad_blocks[i] == bbnum)
        {

            return true;
        }
    }

    return false;
}
