#include "data_acquire.h"

#include "nrf_drv_saadc.h"
#include "app_timer.h"
#include "nrf_queue.h"
#include "AFE_connect.h"
#include "MC36XX.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "fds.h"
#include "nand_spi_flash.h"
#include "data_storage.h"
#include "ble_nus.h"
#include "custom_board.h"
#include "data_fds.h"

#define QUEUE_SIZE 12

NRF_QUEUE_DEF(int16_t, flash_ecg_queue, QUEUE_SIZE * 5, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, flash_accx_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, flash_accy_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, flash_accz_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, flash_ppgr_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, flash_ppgir_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);

NRF_QUEUE_DEF(int16_t, rt_ecg_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, rt_accx_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, rt_accy_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, rt_accz_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, rt_ppgr_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);
NRF_QUEUE_DEF(int16_t, rt_ppgir_queue, QUEUE_SIZE, NRF_QUEUE_MODE_OVERFLOW);

APP_TIMER_DEF(millis_timer);
APP_TIMER_DEF(fastACQ_timer);
APP_TIMER_DEF(slowACQ_timer);
APP_TIMER_DEF(log_timer);

int64_t millis = 0;
int64_t settime = 0;
int16_t spo2 = 2048;
int16_t heartRate = 0;
int16_t bodytemp = 1024;

bool in_rt_mode = false;
bool in_flash_send_mode = false;
bool is_connected = false;
bool acq_is_working = false;
bool flash_write_full = false;

bool force_acq_mode = true;

int16_t leads_off_volt = 0;

extern nand_flash_addr_t flash_offset;
extern nand_flash_addr_t flash_read;
extern nand_flash_badblocks_t flash_badblocks;

extern fds_record_t const m_flash_offset_record;
extern fds_record_t const m_flash_read_record;
extern fds_record_t const m_flash_bad_block_record;

extern fds_record_desc_t flash_offset_desc;
extern fds_record_desc_t flash_read_desc;
extern fds_record_desc_t flash_badblock_desc;

ret_code_t ble_data_send(uint8_t* sendbuf, uint16_t llength);

static void m_millis_timer_handler(void *p_context)
{
    millis++;
}

static void m_fastACQ_timer_handler(void *p_context)
{
		int16_t val;
		static int state_counter = 0;
    nrf_saadc_value_t saadc_val;
	
    nrfx_saadc_sample_convert(1, &saadc_val);
		
//		static int offset = 0;
//		saadc_val = tst_ecg[offset++];
//		if(offset == 500) offset = 0;
	
    nrf_queue_push(&flash_ecg_queue, &saadc_val);
	
		if(in_rt_mode && (state_counter == 0))
			nrf_queue_push(&rt_ecg_queue, &saadc_val);
	
		switch(state_counter){
			case 0:
				val = MC36XXreadXAccel();
				nrf_queue_push(&flash_accx_queue, &val);
				if(in_rt_mode)
					nrf_queue_push(&rt_accx_queue, &val);
				break;
			case 1:
				val = MC36XXreadYAccel();
				nrf_queue_push(&flash_accy_queue, &val);
				if(in_rt_mode)
					nrf_queue_push(&rt_accy_queue, &val);
				break;
			case 2:
				val = MC36XXreadZAccel();
				nrf_queue_push(&flash_accz_queue, &val);
				if(in_rt_mode)
					nrf_queue_push(&rt_accz_queue, &val);
				break;
			case 3:
				val = AFE_Reg_Read_int16(LED1VAL); //IR
				nrf_queue_push(&flash_ppgr_queue, &val);
				if(in_rt_mode)
					nrf_queue_push(&rt_ppgr_queue, &val);
				break;
			case 4:
				val = AFE_Reg_Read_int16(LED2VAL); //Red
				nrf_queue_push(&flash_ppgir_queue, &val);
				if(in_rt_mode)
					nrf_queue_push(&rt_ppgir_queue, &val);
				break;
		
		}
		
		state_counter++;
		if(state_counter == 5)
			state_counter = 0;
		
}

static void m_slowACQ_timer_handler(void *p_context)
{
		//Handling bodytemp, battery voltage
		uint32_t err_code;
		
		if(acq_is_working)
			nrfx_saadc_sample_convert(0, &bodytemp);
		
		//nrfx_saadc_sample_convert(2, &leads_off_volt);
		leads_off_volt = nrf_gpio_pin_read(LEADS_OFF_PIN);
		
		if(force_acq_mode)
		  leads_off_volt = 0;
	
		if((leads_off_volt == 0) && (!acq_is_working)) {
				
				MC36XXSetMode(MC36XX_MODE_CWAKE);
				AFE_enable();
				err_code = app_timer_start(fastACQ_timer, APP_TIMER_TICKS(2), NULL); 		//500Hz ECG, ACC, SpO2
				APP_ERROR_CHECK(err_code);
				acq_is_working = true;
			
		}
}

static void m_log_timer_handler(void *p_context)
{

		static fds_stat_t stat = {0};
		 
		ret_code_t ret = fds_stat(&stat);
		APP_ERROR_CHECK(ret);
		
		NRF_LOG_INFO("FDS words available:%d", stat.freeable_words);
	
    NRF_LOG_INFO("Writing block %d, page %d, column %d", flash_offset.block, flash_offset.page, flash_offset.column);
    NRF_LOG_INFO("send success block %d, page %d, column %d", flash_read.block, flash_read.page, flash_read.column);
	
}

void timers_create(void)
{
		ret_code_t err_code;
		err_code = app_timer_create(&millis_timer, APP_TIMER_MODE_REPEATED, m_millis_timer_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&fastACQ_timer, APP_TIMER_MODE_REPEATED, m_fastACQ_timer_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&slowACQ_timer, APP_TIMER_MODE_REPEATED, m_slowACQ_timer_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&log_timer, APP_TIMER_MODE_REPEATED, m_log_timer_handler);
    APP_ERROR_CHECK(err_code);

}

void timers_start(void)
{
    uint32_t err_code;
    err_code = app_timer_start(millis_timer, APP_TIMER_TICKS(1), NULL);
    APP_ERROR_CHECK(err_code);

//    err_code = app_timer_start(fastACQ_timer, APP_TIMER_TICKS(2), NULL); 		//500Hz ECG, ACC, SpO2
//    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(slowACQ_timer, APP_TIMER_TICKS(1000), NULL); 	//1Hz Bodytemp, Battery
    APP_ERROR_CHECK(err_code);

//    err_code = app_timer_start(log_timer, APP_TIMER_TICKS(1000), NULL);
//    APP_ERROR_CHECK(err_code);
}

static void saadc_callback(nrf_drv_saadc_evt_t const *p_event)
{
}

void saadc_init(void)
{

    ret_code_t err_code;

    nrf_saadc_channel_config_t channel_temperature = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1);
	
		channel_temperature.gain = NRF_SAADC_GAIN1_3;  //1.8V

    //nrf_saadc_channel_config_t channel_battery = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);
    //nrf_saadc_channel_config_t channel_leadsoff = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN4);

    nrf_saadc_channel_config_t channel_ecg = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ADC_ECG_CHANNEL);

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_temperature);
    APP_ERROR_CHECK(err_code);
	
		err_code = nrf_drv_saadc_channel_init(1, &channel_ecg);
    APP_ERROR_CHECK(err_code);
	
		nrf_gpio_cfg_input(LEADS_OFF_PIN, NRF_GPIO_PIN_NOPULL);

}
////////////////////////////////////////////////////////////////////////////////////////
int16_t rt_send_buffer[122];
int16_t flash_write_buffer[120];
uint8_t flash_read_buffer[194];

void ble_rt_send(void)
{
		static int ble_rt_send_offset = 0;
    ret_code_t ret = nrf_queue_pop(&rt_ecg_queue, &rt_send_buffer[ble_rt_send_offset * 6]);
    if (ret == NRF_SUCCESS)
    {
        nrf_queue_pop(&rt_ppgr_queue, &rt_send_buffer[ble_rt_send_offset * 6 + 1]);
        nrf_queue_pop(&rt_ppgir_queue, &rt_send_buffer[ble_rt_send_offset * 6 + 2]);
        nrf_queue_pop(&rt_accx_queue, &rt_send_buffer[ble_rt_send_offset * 6 + 3]);
        nrf_queue_pop(&rt_accy_queue, &rt_send_buffer[ble_rt_send_offset * 6 + 4]);
        nrf_queue_pop(&rt_accz_queue, &rt_send_buffer[ble_rt_send_offset * 6 + 5]);

        ble_rt_send_offset++;
    }

    if (ble_rt_send_offset == 20)
    {
        rt_send_buffer[120] = spo2;
        rt_send_buffer[121] = bodytemp;
				ble_data_send((uint8_t*)rt_send_buffer, 244);
        ble_rt_send_offset = 0;
    }
}

void nand_flash_data_write(void)
{
		int errid = 0;
    ret_code_t ret;
		static uint16_t flash_write_data_offset = 0;
    ret = nrf_queue_pop(&flash_ppgir_queue, &flash_write_buffer[flash_write_data_offset * 10 + 9]); //check the last acquired queue
    if (ret == NRF_SUCCESS) //When all data is acquired
    {
				nrf_queue_pop(&flash_ecg_queue, &flash_write_buffer[flash_write_data_offset * 10 + 8]);		//ECG
        nrf_queue_pop(&flash_ppgr_queue, &flash_write_buffer[flash_write_data_offset * 10 + 7]);
				nrf_queue_pop(&flash_ecg_queue, &flash_write_buffer[flash_write_data_offset * 10 + 6]);		//ECG
				nrf_queue_pop(&flash_accz_queue, &flash_write_buffer[flash_write_data_offset * 10 + 5]);
				nrf_queue_pop(&flash_ecg_queue, &flash_write_buffer[flash_write_data_offset * 10 + 4]);		//ECG
				nrf_queue_pop(&flash_accy_queue, &flash_write_buffer[flash_write_data_offset * 10 + 3]);
				nrf_queue_pop(&flash_ecg_queue, &flash_write_buffer[flash_write_data_offset * 10 + 2]);		//ECG
				nrf_queue_pop(&flash_accx_queue, &flash_write_buffer[flash_write_data_offset * 10 + 1]);
				nrf_queue_pop(&flash_ecg_queue, &flash_write_buffer[flash_write_data_offset * 10]);       //ECG

        flash_write_data_offset++;

        if (flash_write_data_offset == 12)
        {
            if (flash_offset.page == 0 && flash_offset.column == 0) //needs to be write but block not erased
            {

                errid = NSF_ERR_ERASE;
                while (errid == NSF_ERR_ERASE)
                {

                    NRF_LOG_INFO("erasing block %d", flash_offset.block);
                    errid = nand_spi_flash_block_erase(flash_offset.block << 6);
                    if (errid == NSF_ERR_ERASE)
                    {

                        NRF_LOG_INFO("found bad block %d", flash_offset.block);
                        flash_badblocks.bad_blocks[flash_badblocks.bad_block_num++] = flash_offset.block; //store the bad block
											
												//Writing to FDS
												nand_fds_update(&flash_badblock_desc, &m_flash_bad_block_record);
												NRF_LOG_INFO("FDS updated");

                        flash_offset.block++;

                        if (flash_offset.block == 2048)
                        {
                            flash_write_full = true;
														return;
                        }
                    }
                }
            }

            errid = nand_spi_flash_page_write((flash_offset.block << 6) | flash_offset.page, flash_offset.column, (uint8_t *)flash_write_buffer, 240);
            //NRF_LOG_INFO("Writing block %d, page %d, column %d, size %d, %s", flash_offset.block, flash_offset.page, flash_offset.column, 240, nand_spi_flash_str_error(errid));
            flash_offset.column += 240;
            flash_write_data_offset = 0;
        }

        if ((flash_offset.column == 4080) && (flash_write_data_offset == 6)) //this indicates the true column number is 4080, the rest data need to be stored
        {

            flash_write_buffer[60] = bodytemp;			//4202-4203
						
						*(int64_t*)(&flash_write_buffer[61]) = millis;
//						flash_write_buffer[61] = millis >> 48;	//4204-4205
//						flash_write_buffer[62] = millis >> 32;	//4206-4207
//						flash_write_buffer[63] = millis >> 16;	//4208-4209
//						flash_write_buffer[64] = millis;				//4210-4211
					
            //*(uint32_t *)&flash_write_buffer[70] = millis;  //4221-4224
            errid = nand_spi_flash_page_write((flash_offset.block << 6) | flash_offset.page, flash_offset.column, (uint8_t *)flash_write_buffer, 132);
            //NRF_LOG_INFO("Writing block %d, page %d, column %d, size %d, %s", flash_offset.block, flash_offset.page, flash_offset.column, 144, nand_spi_flash_str_error(errid));
            flash_offset.column = 0;
            flash_write_data_offset = 0;
            flash_offset.page++;
					
						if((leads_off_volt == 1) && (acq_is_working)) {
				
								
								ret = app_timer_stop(fastACQ_timer); 		//500Hz ECG, ACC, SpO2
								APP_ERROR_CHECK(ret);
								MC36XXSetMode(MC36XX_MODE_STANDBY);
								AFE_shutdown();
								acq_is_working = false;
							
						}
						
						//Writing to FDS
						nand_fds_update(&flash_offset_desc, &m_flash_offset_record);
						//NRF_LOG_INFO("FDS updated");
						
            if (flash_offset.page == 64)
            {
							  //gc every pages full
								fds_gc_process();

                flash_offset.page = 0;
                flash_offset.block++;
                if (flash_offset.block == 2048)
                {
                    flash_write_full = true;
                    //flash_write_cycle++;
                    //flash_offset.block = 0;
                }
            }
        }
    }
}

#define NAND_ADDR(BLOCKADDR, PAGEADDR) ((((uint32_t)BLOCKADDR) << 6) | PAGEADDR)
bool is_read = false;

void nand_flash_data_read(void)
{
		//bool ret_flag = false;
    int errid = 0;
    ret_code_t ret;
    if ((NAND_ADDR(flash_read.block, flash_read.page) < NAND_ADDR(flash_offset.block, flash_offset.page)))
    {

        if (!is_read)
        {
            errid = nand_spi_flash_page_read((flash_read.block << 6) | flash_read.page, flash_read.column, flash_read_buffer + 2, 192);
            is_read = true;
        

						*(int16_t*)flash_read_buffer = flash_read.column / 192;
						if(flash_read.column == 4032)
						{
								//NRF_LOG_INFO("172 %x %x %x %x", flash_read_buffer[172], flash_read_buffer[173], flash_read_buffer[174], flash_read_buffer[175]);
								uint8_t millis_buf[8];
								memcpy(millis_buf, flash_read_buffer + 172, 8);
								int64_t* read_millis = (int64_t*)millis_buf;
								*read_millis = *read_millis + settime;
								NRF_LOG_INFO("read_millis %u",(uint32_t)((*read_millis) / 1000));
								memcpy(flash_read_buffer + 172, millis_buf, 8);
								//*(int64_t*)(&flash_read_buffer[172]) = read_millis;
						}
				}		
				
				ret = ble_data_send(flash_read_buffer, 194);
				
        if (ret == NRF_SUCCESS)
        {
            NRF_LOG_INFO("send success block %d, page %d, column %d, size %d, %s", flash_read.block, flash_read.page, flash_read.column, 192, nand_spi_flash_str_error(errid));
            is_read = false;
            flash_read.column += 192;
            if (flash_read.column == 4224)
            {

                flash_read.column = 0;
                flash_read.page++;
							
								//Writing to FDS
								nand_fds_update(&flash_read_desc, &m_flash_read_record);

                if (flash_read.page == 64)
                {
										//gc every pages full
										fds_gc_process();
									
                    flash_read.page = 0;
                    flash_read.block++;

                    if (flash_read.block == 2048)
                    {
                        //Read completed, return to block 0
                        flash_offset.block = 0; //TODO: check the block 0
                        flash_write_full = false;
                    }

                    while (is_bad_block_existed(flash_read.block))
                    {
                        NRF_LOG_INFO("read bad block jumped %d", flash_read.block);
                        flash_read.block++;
                        if (flash_read.block == 2048)
                        {
                            flash_offset.block = 0; //TODO: check the block 0
                            flash_write_full = false;
                        }
                    }
                }
            }
        }
    }
		//return ret_flag;
}
