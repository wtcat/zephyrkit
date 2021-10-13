/*==============================================================================
* Edit History
*
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
*
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-04-12 bh        Add license information and revision information
* 2016-04-07 bh        Initial revision.
==============================================================================*/

#include "pah_comm.h"

// platform support
#include "i2c.h"

#ifdef _ALGO_NON_USE_HEAP_SIZE
uint32_t pah_alg_buffer[PAH_ALG_MAX_SIZE / sizeof(uint32_t)];
#endif

/*============================================================================
STATIC VARIABLE DEFINITIONS
============================================================================*/

// valid bank range: 0x00 ~ 0x03
static uint8_t _curr_bank = 0xFF;

/*============================================================================
PUBLIC FUNCTION DEFINITIONS
============================================================================*/
bool pah_comm_write(uint8_t addr, uint8_t data)
{
	if (addr == 0x7F) {
		if (_curr_bank == data)
			return true;

		pah8112_i2c_write(0x7F, &data, 1);
		_curr_bank = data;
		return true;
	}

	if (addr != 0x7F)
		debug_printf("B:%x,R:0x%02x,D:0x%02x\n", _curr_bank, addr, data);
	pah8112_i2c_write(addr, &data, 1);
	return true;
}

bool pah_comm_read(uint8_t addr, uint8_t *data)
{
	pah8112_i2c_read(addr, data, 1);
	return true;
}

bool pah_comm_burst_read(uint8_t addr, uint8_t *data, uint8_t num)
{
	pah8112_i2c_read(addr, data, num);
	return true;
}
bool pah_comm_burst_read_fifo(uint8_t addr, uint8_t *data, uint16_t num, uint8_t *cks_out)
{
	static const uint16_t rx_size_per_read = 252;
	uint16_t read_index = 0;
	uint8_t i = 0;
	uint8_t cks[4] = {0};
	uint8_t cks_tmp[4] = {0};
	while (num) {
		uint16_t read_size = rx_size_per_read;
		if (num < rx_size_per_read)
			read_size = num % rx_size_per_read;

		pah_comm_write(0x7F, 0x03);
		pah_comm_burst_read(addr, &data[read_index], read_size);
		pah_comm_write(0x7F, 0x02);
		pah_comm_burst_read(0x1C, cks_tmp, 4); // checksum

		for (i = 0; i < 4; i++) {
			cks[i] ^= cks_tmp[i];
		}

		read_index += rx_size_per_read;
		num -= read_size;
	}
	for (i = 0; i < 4; i++) {
		cks_out[i] = cks[i];
	}

	return true;
}
