/*==============================================================================
* Edit History
* 
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
* 
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-04-12 bh        - Add license information and revision information
* 2016-04-07 bh        - Initial revision.
==============================================================================*/

#include "pah_comm.h"

// platform support
#include "pah_platform_functions.h"


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
    if (addr == 0x7F)
    {
        if (_curr_bank == data)
            return true;

        if (!spi_write_reg(0x7F, data))
            return false;

        _curr_bank = data;
        return true;
    }

    addr &= (0x7F); //write, bit7 = 0

	if(addr!=0x7F)
	debug_log("B:%x,R:%x,D:%x\n",_curr_bank,addr,data);

    return spi_write_reg(addr, data);
}

bool pah_comm_read(uint8_t addr, uint8_t *data)
{
    addr |= (0x80);	//read, bit7 = 1

    return spi_read_reg(addr, data);
}

bool pah_comm_burst_read(uint8_t addr, uint8_t *data, uint8_t num)
{
    addr |= (0x80);	//read, bit7 = 1

    return spi_burst_read_reg(addr, data, num);
}
