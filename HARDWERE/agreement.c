#include "agreement.h"
#include "nand.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

static uint8_t s_readbuf[2048] = {0};
static uint8_t s_comparebuf[2048] = {0};

uint32_t check_is_to_rec_file(uint8_t *buf)
{

	uint32_t filesize = 0;
	if(buf[0] != buf[1] || buf[0] != ':') {
		return 0;
	}
	for(uint8_t i = 2; i <= 5 ; i++) {
		filesize = (filesize << 8) | buf[i];
	}
	return filesize;
}

uint32_t get_file_startaddr(uint8_t *buf) {
	uint32_t start_addr = 0xFFFFFFFF;
	uint8_t readbuf[256] = {0};
	if(buf[0] != buf[1] || buf[0] != ':') {
		return 0;
	}
	for(uint8_t i = 6; i <= 9 ; i++) {
		start_addr = (start_addr << 8) | buf[i];
	}
	
	nand_read_everywhere(start_addr,readbuf,256,NAND_MAIN_AREA);
		for (uint16_t i = 0; i < 256 ; i++) {
			if(readbuf[i] != 0xff) {
				start_addr = 0;
				return 0xFFFFFFFF;
			}
		}
	
	return start_addr;
	
}

void erase_which_block(uint8_t *buf) 
{
	if(buf[0] != buf[1] || buf[0] != 'C') {
		return ;
	}
	uint16_t erasenum = 0;
	for(uint8_t i = 5; i <= 6; i++ ) {
		erasenum = (erasenum << 8) | buf[i];
	}
	/*擦多页*/
	uint16_t start_erase =0;
	for(uint8_t i = 3; i <= 4; i++ ) {
		start_erase = (start_erase << 8) | buf[i];
	}
	
	if(buf[2] != 0 && erasenum != 0) {
		for(uint16_t i = start_erase; i < erasenum ; i++ ) {
			nand_block_erase(i);
		}
		printf("%d--%d块擦除完成\r\n",start_erase,(erasenum - 1));
	}
	/*擦单页*/
	else if(buf[2] == 0) {
		nand_block_erase(erasenum);
		printf("%d页擦除完成\r\n",erasenum);
	}
}

void nand_page_state(uint8_t *buf)
{
	if(buf[0] != buf[1] || buf[0] != 'S') {
		return ;
	}
	uint32_t i = 0;
	
	memset(s_comparebuf,0xff,2048);
	for(; i < 262144; i++ ) {
		nand_read_everywhere((i*2048),s_readbuf,2048,NAND_MAIN_AREA);
		if(memcmp(s_readbuf,s_comparebuf,2048) == 0) {
			memset(s_readbuf,0,2048);
			break;
		}
	}
	printf("当前第%d页为空\r\n", i+1);
	printf("可以选择在%x地址处开始写入\r\n",(i) * 2048);
}
