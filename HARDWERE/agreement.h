#ifndef AGREEMENT_H
#define AGREEMENT_H

#include "stm32h7xx_hal.h"

uint32_t check_is_to_rec_file(uint8_t *buf);
uint32_t get_file_startaddr(uint8_t *buf);
void erase_which_block(uint8_t *buf);
void nand_page_state(uint8_t *buf);

#endif /*AGREEMENT_H*/
