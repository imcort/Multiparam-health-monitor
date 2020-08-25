#ifndef _DATA_FDS_H__
#define _DATA_FDS_H__

#include <stdint.h>
#include <stdbool.h>

void nand_fds_open(void* desc, void* dest, int size);
void nand_fds_write(void* desc, const void* src);
void nand_fds_update(void* desc, const void* src);
void fds_gc_process(void);
void fds_prepare(void);

bool is_bad_block_existed(uint16_t bbnum);

#endif
