#ifndef __DATA_STORAGE_H__
#define __DATA_STORAGE_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct
{

    uint16_t column;
    uint16_t page;
    uint16_t block;
		uint16_t __not_used_;

} nand_flash_addr_t;

typedef struct
{

    uint16_t bad_blocks[40];
		uint16_t bad_block_num;

} nand_flash_badblocks_t;

void fds_prepare(void);
void nand_flash_prepare(void);
void nand_fds_update(void* desc, const void* src);
void fds_gc_process(void);
bool is_bad_block_existed(uint16_t bbnum);


#endif
