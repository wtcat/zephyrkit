/*
 * mcu_example.c
 * This software package is not part of Parade's Linux touchscreen driver, and
 * was created as an example only, to work without the support of any OS. This
 * package describes:
 *  1) How to communicate with Parade Touch Chips (Gen5 and Gen6)
 *  2) How to parse touch report message
 *  3) How to update a whole firmware and a configure file
 *  4) How to perform MFG test: Auto Shorts, CM, CP selftest
 *  5) How to print MFG test result in unified format
 *
 * Copyright (c) 2015-2020, Parade Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the True Touch Driver for Linux
 * (TTDL) project.
 *
 * Contact Parade Technologies at www.paradetech.com <ttdrivers@paradetech.com>
 */

#define UPDATE_FW_BY_BIN

#if defined(UPDATE_FW_BY_HEADER)
#include "fw.h"
#include "fw_conf.h"
#elif defined(UPDATE_FW_BY_BIN)
//#include "tma525.H"
#endif
#include "pt_mcu_port.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"
#include <kernel.h>
#include <sys/atomic.h>
#include <sys/printk.h>



/* comment this macro when finish the debug */
//#define PRINT_PIP_MSG
//#define PRINT_DBG_MSG

#define TX_IN_X_DIRECTION // TX_IN_Y_DIRECTION
#define TX_NUM (7)
#define RX_NUM (7)
/* File Path for MFG test */
#define THRESHOLD_SUPPORT (1)
#define THESHOLD_FILE ("/data/cmcp_threshold_file.csv")
#define RESULT_FILE_PATH ("/data/")
/* Address */
#define I2C_ADDR (0x24)
/* Limit */
#define PT_MAX_PIP_MSG_SIZE 264
#define PT_MAX_INPUT PT_MAX_PIP_MSG_SIZE
#define PT_DATA_MAX_ROW_SIZE 256
#define PT_DATA_ROW_SIZE 128
#define PIP_CMD_MAX_LENGTH ((1 << 16) - 1)
#define MAX_BUF_LEN (10 * 1024)

/* Low-level funcitons */
/* Delay functions */
#define DELAY_MS(ms) pt_delay_ms(ms)
#define DELAY_US(us) pt_delay_us(us)

/* Power functions */
#define POWER_ON_2V8()                                                                                                 \
	do {                                                                                                               \
		pt_set_vdda_gpio(1);                                                                                           \
	} while (0)
#define POWER_ON_1V8()                                                                                                 \
	do {                                                                                                               \
		pt_set_vddd_gpio(1);                                                                                           \
	} while (0)

/* I2C function*/
#define I2C_READ(read_buf, read_len) pt_i2c_read(I2C_ADDR, read_buf, read_len, PT_MAX_PIP_MSG_SIZE)
#define I2C_WRITE(write_buf, write_len) pt_i2c_write(I2C_ADDR, write_buf, write_len)

/* GPIO functions */
#define GPIO_HIGH (1)
#define GPIO_LOW (0)

#define GPIO_SET_RST_MODE_OUTPUT()
#define GPIO_SET_RST_HIGH()                                                                                            \
	do {                                                                                                               \
		pt_set_rst_gpio(1);                                                                                            \
	} while (0)
#define GPIO_SET_RST_LOW()                                                                                             \
	do {                                                                                                               \
		pt_set_rst_gpio(0);                                                                                            \
	} while (0)

#define GPIO_SET_INT_MODE_INPUT()                                                                                      \
	do {                                                                                                               \
		pt_set_int_gpio_mode();                                                                                        \
	} while (0)
#define GPIO_READ_INT_STATE() pt_get_irq_state()
#define REGISTER_INT_HANDLER(handler)                                                                                  \
	do {                                                                                                               \
		add_touch_handler(handler);                                                                                    \
	} while (0)
#define REGISTER_60HZ_HANDLER(handler)                                                                                 \
	do {                                                                                                               \
		add_touch_poller_60hz(handler);                                                                                \
	} while (0)

#define DISABLE_IRQ()                                                                                                  \
	do {                                                                                                               \
		pt_disable_irq();                                                                                              \
	} while (0)
#define ENABLE_IRQ()                                                                                                   \
	do {                                                                                                               \
		pt_enable_irq();                                                                                               \
	} while (0)
#define GET_SYSTEM_TICK() (pt_get_time_stamp())

//#define FILE_FLAG_CREATE (O_CREAT | O_TRUNC)
//#define FILE_FLAG_READ (O_RDONLY)
//#define FILE_FLAG_WRITE (O_WRONLY)
#define FILE_RET_SUC (0)
//#define FILE_OPEN(file_path, flags) pt_open_file(file_path, flags)
#define FILE_RENAME(old_file, new_file) pt_rename_file(old_file, new_file)
#define FILE_CLOSE() pt_close_file()
#define FILE_WRITE(wbuf, wlen) pt_write_file(wbuf, wlen)
#define FILE_READ(rbuf, rlen) pt_read_file(rbuf, rlen)
#define FILE_SEEK(offset) pt_file_seek(offset)
#define FILE_SIZE() pt_file_size()

/* ASCII */
#define ASCII_LF (0x0A)
#define ASCII_CR (0x0D)
#define ASCII_COMMA (0x2C)
#define ASCII_ZERO (0x30)
#define ASCII_NINE (0x39)
/* PIP1 offsets and masks */
#define PT_CMD_OUTPUT_REGISTER 0x0004
#define PIP1_RESP_REPORT_ID_OFFSET 2
#define PIP1_RESP_COMMAND_ID_OFFSET 4
#define PIP1_RESP_COMMAND_ID_MASK 0x7F
#define PIP1_CMD_COMMAND_ID_OFFSET 6
#define PIP1_CMD_COMMAND_ID_MASK 0x7F

#define PIP1_SYSINFO_TTDATA_OFFSET 5
#define PIP1_SYSINFO_SENSING_OFFSET 33
#define PIP1_SYSINFO_BTN_OFFSET 48
#define PIP1_SYSINFO_BTN_MASK 0xFF
#define PIP1_SYSINFO_MAX_BTN 8

#define HID_APP_REPORT_ID 0xF7
#define HID_BL_REPORT_ID 0xFF
#define HID_RESPONSE_REPORT_ID 0xF0

#define TOUCH_REPORT_SIZE 10
#define TOUCH_INPUT_HEADER_SIZE 7
#define TOUCH_COUNT_BYTE_OFFSET 5

#define PT_POST_TT_CFG_CRC_MASK 0x02

#define PT_ARRAY_ID_OFFSET 0
#define PT_ROW_NUM_OFFSET 1
#define PT_ROW_SIZE_OFFSET 3
#define PT_ROW_DATA_OFFSET 5

// RAM parameters IDs
#define PT_PARAM_FORCE_SINGLE_TX 0x1F
#define PT_PARAM_ACT_LFT_EN 0x1A
#define PT_PARAM_BL_H2O_RJCT 0x05
#define PT_PARAM_LOW_POWER_ENABLE 0x06

// HID POWER STATE
#define HID_POWER_ON 0x0
#define HID_POWER_SLEEP 0x1
#define HID_POWER_STANDBY 0x2

/* helpers */
#define IS_NUMBER(x) ((x) <= '9' && (x) >= '0')
#define IS_ASCII(x) ((x) <= 'Z' && (x) >= 'A')
#define GET_NUM_TOUCHES(x) ((x)&0x1F)
#define GET_NOISE_LEVEL(x) ((x)&0x07)
#define IS_LARGE_AREA(x) ((x)&0x20)
#define HIGH_BYTE(x) (u8)(((x) >> 8) & 0xFF)
#define LOW_BYTE(x) (u8)((x)&0xFF)
#define CHECK_PIP1_RESP_ID(rsp_buf, cmd_id) (((rsp_buf)[2] == 0x1F) && (((rsp_buf)[4] & 0x7f) == (cmd_id)))
#define CHECK_PIP1_RESP_ID_AND_STATUS(rsp_buf, cmd_id)                                                                 \
	(((rsp_buf)[2] == 0x1F) && (((rsp_buf)[4] & 0x7f) == (cmd_id)) && ((rsp_buf)[5] == 0x00))

#ifdef PRINT_DBG_MSG
#define PT_DBG(fmt, arg...) printk(fmt, ##arg)
#else
#define PT_DBG(fmt, arg...)
#endif

#if 0
#define PT_ERR(fmt, arg...) printk(fmt, ##arg)
#else
#define PT_ERR(...) printk(__VA_ARGS__)
#endif
enum hid_command {
	HID_CMD_RESERVED = 0x0,
	HID_CMD_RESET = 0x1,
	HID_CMD_GET_REPORT = 0x2,
	HID_CMD_SET_REPORT = 0x3,
	HID_CMD_GET_IDLE = 0x4,
	HID_CMD_SET_IDLE = 0x5,
	HID_CMD_GET_PROTOCOL = 0x6,
	HID_CMD_SET_PROTOCOL = 0x7,
	HID_CMD_SET_POWER = 0x8,
	HID_CMD_VENDOR = 0xE,
};

enum pt_ic_ebid {
	PT_TCH_PARM_EBID = 0x00,
	PT_MDATA_EBID = 0x01,
	PT_DDATA_EBID = 0x02,
	PT_CAL_EBID = 0xF0,
};

enum pt_mode {
	PT_MODE_UNKNOWN = 0,
	PT_MODE_BOOTLOADER = 1,
	PT_MODE_OPERATIONAL = 2,
	PT_MODE_IGNORE = 255,
};

/* PIP BL cmd IDs and input for dut_debug sysfs */
enum pip1_bl_cmd_id {
	PIP1_BL_CMD_ID_VERIFY_APP_INTEGRITY = 0x31,
	PIP1_BL_CMD_ID_GET_INFO = 0x38,
	PIP1_BL_CMD_ID_PROGRAM_AND_VERIFY = 0x39,
	PIP1_BL_CMD_ID_LAUNCH_APP = 0x3B,
	PIP1_BL_CMD_ID_GET_PANEL_ID = 0x3E,
	PIP1_BL_CMD_ID_INITIATE_BL = 0x48,
	PIP1_BL_CMD_ID_LAST,
};

/* PIP1 Command/Response IDs */
enum PIP1_CMD_ID {
	PIP1_CMD_ID_NULL = 0x00,
	PIP1_CMD_ID_START_BOOTLOADER = 0x01,
	PIP1_CMD_ID_GET_SYSINFO = 0x02,
	PIP1_CMD_ID_SUSPEND_SCANNING = 0x03,
	PIP1_CMD_ID_RESUME_SCANNING = 0x04,
	PIP1_CMD_ID_GET_PARAM = 0x05,
	PIP1_CMD_ID_SET_PARAM = 0x06,
	PIP1_CMD_ID_GET_NOISE_METRICS = 0x07,
	PIP1_CMD_ID_RESERVED = 0x08,
	PIP1_CMD_ID_ENTER_EASYWAKE_STATE = 0x09,
	PIP1_CMD_ID_EXIT_EASYWAKE_STATE = 0x10,
	PIP1_CMD_ID_VERIFY_CONFIG_BLOCK_CRC = 0x20,
	PIP1_CMD_ID_GET_CONFIG_ROW_SIZE = 0x21,
	PIP1_CMD_ID_READ_DATA_BLOCK = 0x22,
	PIP1_CMD_ID_WRITE_DATA_BLOCK = 0x23,
	PIP1_CMD_ID_GET_DATA_STRUCTURE = 0x24,
	PIP1_CMD_ID_LOAD_SELF_TEST_PARAM = 0x25,
	PIP1_CMD_ID_RUN_SELF_TEST = 0x26,
	PIP1_CMD_ID_GET_SELF_TEST_RESULT = 0x27,
	PIP1_CMD_ID_CALIBRATE_IDACS = 0x28,
	PIP1_CMD_ID_INITIALIZE_BASELINES = 0x29,
	PIP1_CMD_ID_EXEC_PANEL_SCAN = 0x2A,
	PIP1_CMD_ID_RETRIEVE_PANEL_SCAN = 0x2B,
	PIP1_CMD_ID_START_SENSOR_DATA_MODE = 0x2C,
	PIP1_CMD_ID_STOP_SENSOR_DATA_MODE = 0x2D,
	PIP1_CMD_ID_START_TRACKING_HEATMAP_MODE = 0x2E,
	PIP1_CMD_ID_START_SELF_CAP_RPT_MODE = 0x2F,
	PIP1_CMD_ID_CALIBRATE_DEVICE_EXTENDED = 0x30,
	PIP1_CMD_ID_INT_PIN_OVERRIDE = 0x40,
	PIP1_CMD_ID_STORE_PANEL_SCAN = 0x60,
	PIP1_CMD_ID_PROCESS_PANEL_SCAN = 0x61,
	PIP1_CMD_ID_DISCARD_INPUT_REPORT,
	PIP1_CMD_ID_LAST,
	PIP1_CMD_ID_USER_CMD,
};

enum PT_PIP_REPORT_ID {
	PT_PIP_INVALID_REPORT_ID = 0x00,
	PT_PIP_TOUCH_REPORT_ID = 0x01,
	PT_PIP_TOUCH_REPORT_WIN8_ID = 0x02,
	PT_PIP_CAPSENSE_BTN_REPORT_ID = 0x03,
	PT_PIP_WAKEUP_REPORT_ID = 0x04,
	PT_PIP_NOISE_METRIC_REPORT_ID = 0x05,
	PT_PIP_PUSH_BUTTON_REPORT_ID = 0x06,
	PT_PIP_SELFCAP_INPUT_REPORT_ID = 0x0D,
	PT_PIP_TRACKING_HEATMAP_REPORT_ID = 0x0E,
	PT_PIP_SENSOR_DATA_REPORT_ID = 0x0F,
	PT_PIP_NON_HID_RESPONSE_ID = 0x1F,
	PT_PIP_NON_HID_COMMAND_ID = 0x2F,
	PT_PIP_BL_RESPONSE_REPORT_ID = 0x30,
	PT_PIP_BL_COMMAND_REPORT_ID = 0x40
};

enum pt_self_test_id {
	PT_ST_ID_NULL = 0,
	PT_ST_ID_BIST = 1,
	PT_ST_ID_SHORTS = 2,
	PT_ST_ID_OPENS = 3,
	PT_ST_ID_AUTOSHORTS = 4,
	PT_ST_ID_CM_PANEL = 5,
	PT_ST_ID_CP_PANEL = 6,
	PT_ST_ID_CM_BUTTON = 7,
	PT_ST_ID_CP_BUTTON = 8,
	PT_ST_ID_FORCE = 9,
	PT_ST_ID_OPENS_HIZ = 10,
	PT_ST_ID_OPENS_GND = 11,
	PT_ST_ID_CP_LFT = 12,
	PT_ST_ID_INVALID = 255
};

enum DEBUG_LEVEL { DEBUG_LEVEL_0, DEBUG_LEVEL_1, DEBUG_LEVEL_2, DEBUG_LEVEL_3 };
/* Structs */
struct pt_core_data {
	u8 debug_level;
	u8 write_buf[PT_MAX_INPUT];
	u8 read_buf[PT_MAX_INPUT];
	u8 mode;
	u8 system_mode;
	u8 hid_cmd_state;
	u8 pip2_cmd_tag_seq;
	u8 pip2_prot_active;
	u8 pip2_send_user_cmd;
	u8 pip_ver_major;
	u8 pip_ver_minor;
	u8 bl_ver_major;
	u8 bl_ver_minor;
	u8 fw_ver_major;
	u8 fw_ver_minor;
	u8 elec_x; /* tx number when tx == column */
	u8 elec_y; /* rx number when rx == row*/
	u8 mfg_id[8];
	u16 post_code;
	u16 fw_ver_conf;
	u32 revctrl;
};

struct pt_dev_id {
	u32 silicon_id;
	u8 rev_id;
	u32 bl_ver;
};

//#if __linux__
#if 1
struct pt_hex_image {
	u8 array_id;
	u16 row_num;
	u16 row_size;
	u8 row_data[PT_DATA_ROW_SIZE];
} __packed;
#else
__packed struct pt_hex_image {
	u8 array_id;
	u16 row_num;
	u16 row_size;
	u8 row_data[PT_DATA_ROW_SIZE];
};
#endif

/* CMCP test configure file */

struct CMCP_RESULT {
	u8 cm_test_run;
	u8 cp_test_run;
	/* Sensor Cm validation */
	u8 cm_test_pass;
	u8 cm_sensor_validation_pass;
	u8 cm_sensor_row_delta_pass;
	u8 cm_sensor_col_delta_pass;
	u8 cm_sensor_gd_row_pass;
	u8 cm_sensor_gd_col_pass;
	u8 cm_sensor_calibration_pass;
	u8 cm_sensor_delta_pass;
	/* Sensor Cp validation */
	u8 cp_test_pass;
	/*u8 cp_sensor_delta_pass;*/
	u8 cp_sensor_rx_delta_pass;
	u8 cp_sensor_tx_delta_pass;
	/*u8 cp_sensor_average_pass;*/
	u8 cp_rx_validation_pass;
	u8 cp_tx_validation_pass;
	/*other validation*/
	u8 short_test_pass;
	u8 test_summary;
};

typedef enum { RC_PASS = 0, RC_FAIL = -1 } PT_RC_CHECK;

//#if __linux__
#if 1
typedef struct {
	u8 reg[2];
	u16 len;
	u8 seq;
	u8 cmd_id; /* fixed 0*/
} __packed PIP2_CMD_HEADER;

typedef struct {
	u8 reg[2];
	u16 len;
	u8 report_id;
	u8 rsvd;
	u8 cmd_id;
} __packed PIP1_CMD_HEADER;

typedef struct {
	u8 reg[2];
	u16 len;
	u8 report_id;
	u8 rsvd;
	u8 sop;
	u8 cmd_id;
	u16 data_len;
} __packed PIP1_BL_CMD_HEADER;

typedef struct {
	u16 wideband_noise;
	u16 inject_noise;
	u16 NMF_noise;
	u8 touch_type;
	u8 water_level;
	u32 raw_count_variation;
	u32 diff_count_variation;
	u16 CDC_overflow;
	u16 temperature;
} __packed PT_NOISE_METRIC;
#else
typedef __packed struct {
	u8 reg[2];
	u16 len;
	u8 seq;
	u8 cmd_id; /* fixed 0*/
} PIP2_CMD_HEADER;

typedef __packed struct {
	u8 reg[2];
	u16 len;
	u8 report_id;
	u8 rsvd;
	u8 cmd_id;
} PIP1_CMD_HEADER;

typedef __packed struct {
	u8 reg[2];
	u16 len;
	u8 report_id;
	u8 rsvd;
	u8 sop;
	u8 cmd_id;
	u16 data_len;
} PIP1_BL_CMD_HEADER;

typedef __packed struct {
	u16 wideband_noise;
	u16 inject_noise;
	u16 NMF_noise;
	u8 touch_type;
	u8 water_level;
	u32 raw_count_variation;
	u32 diff_count_variation;
	u16 CDC_overflow;
	u16 temperature;
} PT_NOISE_METRIC;
#endif

static const u8 pt_data_block_security_key[] = {0xA5, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0x5A};

/* global variables */
struct pt_core_data cd = {0};
char mfg_print_buffer[MAX_BUF_LEN]; // To store formated mfg test result
// To store returned cm/cp data (Must bigger than
// PIP2_BL_I2C_FILE_WRITE_LEN_PER_PACKET + 2)
#define IC_BUF_LEN (MAX(300, (TX_NUM * RX_NUM * 2 + 2)))
u8 ic_buf[IC_BUF_LEN];									/* > 264 && TX*RX*2+2 && (TX+RX)*4 */
#define CSV_MAX_LINE (MAX(TX_NUM, RX_NUM) * 7 * 2 + 64) /* > MAX(TX, RX)*7*2 + 33 */
char csv_buffer[CSV_MAX_LINE];
#define THRESHOLD_DATA_MAX (MAX(TX_NUM, RX_NUM) * 2)
u32 threshold_data[THRESHOLD_DATA_MAX];
/* Global Result file name*/
char result_file_name[64];
struct CMCP_RESULT cmcp_result = {0};

#ifdef PRINT_PIP_MSG
#define PR_BUF_SIZE (1024)
char print_buf[PR_BUF_SIZE];
#endif
static void print_pip_msg(u8 *buffer, u8 len, const char *header)
{
#ifdef PRINT_PIP_MSG
	int i = 0;
	int index = 0;

	memset(print_buf, 0, sizeof(print_buf));
	if (header)
		index += sprintf(&print_buf[index], "%s:", header);
	for (i = 0; i < len && index < (PR_BUF_SIZE - 3); i++) {
		index += sprintf(&print_buf[index], " %02X", buffer[i]);
	}
	PT_DBG("%s\n", print_buf);
#endif
}

static u16 get_unaligned_le16(const void *p)
{
	const u8 *_p = p;
	return _p[0] | _p[1] << 8;
}

static u32 get_unaligned_le32(const void *p)
{
	const u8 *_p = p;
	return _p[0] | _p[1] << 8 | _p[2] << 16 | _p[3] << 24;
}

static u16 get_unaligned_be16(const void *p)
{
	const u8 *_p = p;
	return _p[0] << 8 | _p[1];
}

static u32 get_unaligned_be32(const void *p)
{
	const u8 *_p = p;
	return _p[0] << 24 | _p[1] << 16 | _p[2] << 8 | _p[3];
}

/* Methods */
static PT_RC_CHECK pt_read_default_nosize(u8 *buf, u32 max)
{
	u32 size;

	if (!buf)
		return RC_FAIL;

	I2C_READ(buf, 2);
	size = get_unaligned_le16(&buf[0]);
	if (!size) {
		PT_DBG("%s: Sentinel\n", __func__);
		return RC_PASS;
	} else if ((size > max) || (size > PT_MAX_PIP_MSG_SIZE)) {
		PT_DBG("%s: Empty buffer\n", __func__);
		return RC_PASS;
	}

	I2C_READ(buf, size);
	print_pip_msg(buf, size, "READ");
	return RC_PASS;
}

static PT_RC_CHECK pt_write_read_specific(u8 write_len, u8 *write_buf, u8 *read_buf)
{
	PT_RC_CHECK rc = RC_PASS;

	I2C_WRITE(write_buf, write_len);
	print_pip_msg(write_buf, write_len, "WRITE");

	if (read_buf)
		rc = pt_read_default_nosize(read_buf, PT_MAX_PIP_MSG_SIZE);

	return rc;
}

void pt_dut_reset(void)
{
	GPIO_SET_RST_HIGH();
	DELAY_MS(1);
	GPIO_SET_RST_LOW();
	DELAY_MS(10);
	GPIO_SET_RST_HIGH();
}

static const u16 crc_table[16] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};

static u16 _pt_compute_crc(u8 *buf, u32 size)
{
	u16 remainder = 0xFFFF;
	u16 xor_mask = 0x0000;
	u32 index;
	u32 byte_value;
	u32 table_index;
	u32 crc_bit_width = sizeof(u16) * 8;

	/* Divide the message by polynomial, via the table. */
	for (index = 0; index < size; index++) {
		byte_value = buf[index];
		table_index = ((byte_value >> 4) & 0x0F) ^ (remainder >> (crc_bit_width - 4));
		remainder = crc_table[table_index] ^ (remainder << 4);
		table_index = (byte_value & 0x0F) ^ (remainder >> (crc_bit_width - 4));
		remainder = crc_table[table_index] ^ (remainder << 4);
	}

	/* Perform the final remainder CRC. */
	return remainder ^ xor_mask;
}

static PT_RC_CHECK pt_send_pip1_cmd(int cmd_id, u8 *data, u8 data_len)
{
	PT_RC_CHECK rc;
	u16 index = 0;

	PIP1_CMD_HEADER pip1_header = {0};
	pip1_header.reg[0] = LOW_BYTE(PT_CMD_OUTPUT_REGISTER);
	pip1_header.reg[1] = HIGH_BYTE(PT_CMD_OUTPUT_REGISTER);
	/* head.len(2)report id(1) rsvd(1) cmd_id(1) */
	pip1_header.len = data_len + 0x05;
	pip1_header.report_id = 0x2f;
	pip1_header.rsvd = 0x00;
	pip1_header.cmd_id = cmd_id;

	index = 0;
	memcpy(&cd.write_buf[index], &pip1_header, sizeof(pip1_header));
	index += sizeof(pip1_header);

	if (data && data_len) {
		memcpy(&cd.write_buf[index], data, data_len);
		index += data_len;
	}

	rc = pt_write_read_specific(index, cd.write_buf, NULL);
	return rc;
}

static PT_RC_CHECK pt_send_pip1_bl_cmd(int cmd_id, u8 *data, u8 data_len)
{
	PT_RC_CHECK rc;
	u16 index = 0;
	u16 crc;

	PIP1_BL_CMD_HEADER pip1_bl_header = {0};
	pip1_bl_header.reg[0] = LOW_BYTE(PT_CMD_OUTPUT_REGISTER);
	pip1_bl_header.reg[1] = HIGH_BYTE(PT_CMD_OUTPUT_REGISTER);
	/* head.len(2)report id(1) rsvd(1) #sop(1) cmd_id(1) data_len(2) crc(2)
	 * EOP(1) */
	pip1_bl_header.len = data_len + 0x0B;
	pip1_bl_header.report_id = 0x40;
	pip1_bl_header.rsvd = 0x00;
	pip1_bl_header.sop = 0x01; /* SOP */
	pip1_bl_header.cmd_id = cmd_id;
	pip1_bl_header.data_len = data_len;

	index = 0;
	memcpy(&cd.write_buf[index], &pip1_bl_header, sizeof(pip1_bl_header));
	index += sizeof(pip1_bl_header);

	if (data && data_len) {
		memcpy(&cd.write_buf[index], data, data_len);
		index += data_len;
	}
	crc = _pt_compute_crc(&cd.write_buf[6], data_len + 4);
	cd.write_buf[index++] = LOW_BYTE(crc);
	cd.write_buf[index++] = HIGH_BYTE(crc);
	cd.write_buf[index++] = 0x17; /* EOP */

	rc = pt_write_read_specific(index, cd.write_buf, NULL);
	return rc;
}

static int pt_check_irq_asserted(void)
{
	int ret = 0;

	if (GPIO_READ_INT_STATE() == GPIO_LOW)
		DELAY_US(100);
	if (GPIO_READ_INT_STATE() == GPIO_LOW)
		ret = 1;

	PT_DBG("CHECK INT STAT = %s\n", ret ? "LOW" : "HIGH");
	return ret;
}

/* !0-error, 0-success*/
static int pt_flush_bus(void)
{
	int retry = 3;
	int rc = 0;

	while (pt_check_irq_asserted() && (retry-- > 0)) {
		pt_read_default_nosize(cd.read_buf, PT_MAX_PIP_MSG_SIZE);
	}
	if (pt_check_irq_asserted())
		rc = -1;
	else
		rc = 0;
	PT_DBG("FLUSH Bus Result = %s\n", rc ? "Fail" : "Success");
	return rc;
}

int pt_wait_and_clear_int(uint16_t max_wait_ms)
{
	u16 time_ms = 0;
	u8 int_flag = 0;
	int rc = 0;

	while (time_ms < max_wait_ms) {
		if (GPIO_READ_INT_STATE() == GPIO_LOW) {
			int_flag = 1;
		}

		if (int_flag == 1) {
			rc = pt_read_default_nosize(cd.read_buf, PT_MAX_PIP_MSG_SIZE);
			if (!rc) {
				return 1;
			} else {
				PT_ERR("%s: I2C error\n", __func__);
				break;
			}
		}
		DELAY_MS(1);
		time_ms += 1;
	}

	PT_ERR("%s: Time Out\n", __func__);
	return 0;
}

int pt_wait_and_check_cmd_id(uint16_t max_wait_ms, u8 cmd_id)
{
	u16 time_ms = 0;
	u8 int_flag = 0;
	int rc = 0;

	while (time_ms < max_wait_ms) {
		if (GPIO_READ_INT_STATE() == GPIO_LOW) {
			int_flag = 1;
		}

		if (int_flag == 1) {
			rc = pt_read_default_nosize(cd.read_buf, PT_MAX_PIP_MSG_SIZE);
			if (!rc) {
				if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id))
					return 1;
				else {
					PT_ERR("%s: Unexpected msg report_id = %d, retry\n", __func__, cd.read_buf[2]);
					int_flag = 0;
				}
			} else {
				PT_ERR("%s: I2C error\n", __func__);
				break;
			}
		}
		DELAY_MS(1);
		time_ms += 1;
	}

	PT_ERR("%s: Time Out\n", __func__);
	return 0;
}

/**********************************************
 *                 PIP CMD
 **********************************************/
static PT_RC_CHECK pt_pip1_cmd_bl_launch(void)
{
	int rc = 0;
	int int_state;

	rc = pt_send_pip1_bl_cmd(PIP1_BL_CMD_ID_LAUNCH_APP, NULL, 0);
	if (rc < 0)
		PT_ERR("%s: send error\n", __func__);

	int_state = pt_wait_and_clear_int(500);
	if (int_state) {
		if ((cd.read_buf[0] == 0x00) && (cd.read_buf[1] == 0x00)) {
			PT_DBG("%s: Sentinel recevided\n", __func__);
			return RC_PASS;
		}
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_pip1_cmd_bl_get_bl_info(u8 *return_data, u16 max_len)
{
	int rc = 0;
	int int_state;
	u8 cmd_id = PIP1_BL_CMD_ID_GET_INFO;
	u16 data_len;

	rc = pt_send_pip1_bl_cmd(cmd_id, NULL, 0);
	if (rc < 0)
		PT_ERR("%s: send error\n", __func__);

	int_state = pt_wait_and_clear_int(500);
	if (int_state) {
		data_len = get_unaligned_le16(&cd.read_buf[6]);
		memcpy(return_data, &cd.read_buf[8], MIN(data_len, max_len));
		return RC_PASS;
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_pip1_cmd_bl_initiate_bl(u16 row_size, u8 *metadata_row_buf)
{
	int rc = 0;
	int int_state;
	u8 cmd_id = PIP1_BL_CMD_ID_INITIATE_BL;
	u16 write_length = 8 + row_size;
	u8 data_buf[PT_DATA_ROW_SIZE + 8];

	memcpy(&data_buf[0], pt_data_block_security_key, 8);
	memcpy(&data_buf[8], metadata_row_buf, row_size);

	rc = pt_send_pip1_bl_cmd(cmd_id, data_buf, write_length);
	if (rc < 0)
		PT_ERR("%s: send error\n", __func__);

	int_state = pt_wait_and_clear_int(20000);
	if (int_state) {
		return RC_PASS;
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_pip1_cmd_bl_prog_and_verify(u8 *data_buf, u16 data_len)
{
	int rc = 0;
	int int_state;
	u8 cmd_id = PIP1_BL_CMD_ID_PROGRAM_AND_VERIFY;

	rc = pt_send_pip1_bl_cmd(cmd_id, data_buf, data_len);
	if (rc < 0)
		PT_ERR("%s: send error\n", __func__);

	int_state = pt_wait_and_clear_int(500);
	if (int_state) {
		return RC_PASS;
	}

	return RC_FAIL;
}

static int pt_pip1_cmd_get_mode(void)
{
	u8 cmd[2] = {0x01, 0x00}; /* HID CMD */
	u8 mode = 0;

	PT_DBG("%s:enter\n", __func__);
	I2C_WRITE(cmd, 2);
	if (pt_wait_and_clear_int(500)) {
		if (cd.read_buf[PIP1_RESP_REPORT_ID_OFFSET] == HID_APP_REPORT_ID)
			mode = PT_MODE_OPERATIONAL;
		else if (cd.read_buf[PIP1_RESP_REPORT_ID_OFFSET] == HID_BL_REPORT_ID)
			mode = PT_MODE_BOOTLOADER;
		else
			mode = PT_MODE_UNKNOWN;
	}

	PT_DBG("%s: FW mode=%s\n", __func__,
		   (mode == PT_MODE_OPERATIONAL) ? "Operation" : ((mode == PT_MODE_BOOTLOADER) ? "Bootloader" : "Unknown"));
	return mode;
}

static PT_RC_CHECK pt_pip1_cmd_get_sysinfo(void)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_GET_SYSINFO;

	if (cd.mode != PT_MODE_OPERATIONAL) {
		PT_ERR("mode not correct, can't get sysinfo\n");
		return RC_FAIL;
	}

	pt_send_pip1_cmd(cmd_id, NULL, 0);
	int_state = pt_wait_and_check_cmd_id(1000, cmd_id);

	if (int_state) {
		cd.fw_ver_major = cd.read_buf[9];
		cd.fw_ver_minor = cd.read_buf[10];
		cd.revctrl = get_unaligned_le32(&cd.read_buf[11]);
		cd.fw_ver_conf = get_unaligned_le16(&cd.read_buf[15]);
		cd.post_code = get_unaligned_le16(&cd.read_buf[31]);
		cd.elec_x = cd.read_buf[33];
		cd.elec_y = cd.read_buf[34];
		memcpy(cd.mfg_id, &cd.read_buf[23], 8);

		PT_DBG("FW ver=0x%X.%X, rev_ctrl=0x%08X, conf_ver=0x%02X, x=%d, y=%d\n", cd.fw_ver_major, cd.fw_ver_minor,
			   cd.revctrl, cd.fw_ver_conf, cd.elec_x, cd.elec_y);
		return RC_PASS;
	}

	PT_ERR("%s: error\n", __func__);
	return RC_FAIL;
}

static PT_RC_CHECK pt_pip1_cmd_suspend_scan(void)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_SUSPEND_SCANNING;

	pt_send_pip1_cmd(cmd_id, NULL, 0);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id))
			return RC_PASS;
		else
			PT_ERR("%s: Response mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_pip1_cmd_resume_scan(void)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_RESUME_SCANNING;

	pt_send_pip1_cmd(cmd_id, NULL, 0);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id))
			return RC_PASS;
		else
			PT_ERR("%s: Response mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_pip1_cmd_bl_verify_app_integrity(u8 *result)
{
	int int_state;
	u8 cmd_id = PIP1_BL_CMD_ID_VERIFY_APP_INTEGRITY;

	pt_send_pip1_bl_cmd(cmd_id, NULL, 0);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (result)
			*result = cd.read_buf[8];
		return RC_PASS;
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	if (result)
		*result = 0;
	return RC_FAIL;
}

PT_RC_CHECK pt_cmd_hid_power(u8 Powerstate)
{
	int int_state;
	u8 send_data[4] = {0x05, 0x00, 0x00, 0x08};

	send_data[2] = Powerstate;
	pt_write_read_specific(ARRAY_SIZE(send_data), send_data, NULL);
	int_state = pt_wait_and_clear_int(500);

	if (int_state) {
		if (cd.read_buf[2] != HID_RESPONSE_REPORT_ID || (cd.read_buf[3] & 0x03) != Powerstate ||
			((cd.read_buf[4] & 0xF) != HID_CMD_SET_POWER)) {
			PT_ERR("%s: Response mismatch\n", __func__);
			return RC_FAIL;
		}
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
		return RC_FAIL;
	}

	return RC_PASS;
}

PT_RC_CHECK pt_pip1_cmd_enter_easy_wake(void)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_ENTER_EASYWAKE_STATE;
	u8 send_data[1] = {0};

	send_data[0] = 0x00;
	pt_send_pip1_cmd(cmd_id, send_data, 1);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id))
			return RC_PASS;
		else
			PT_ERR("%s: Response mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

PT_RC_CHECK pt_pip1_cmd_exit_easy_wake(void)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_EXIT_EASYWAKE_STATE;

	pt_send_pip1_cmd(cmd_id, NULL, 0);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID_AND_STATUS(cd.read_buf, cmd_id))
			return RC_PASS;
		else
			PT_ERR("%s: Response or Status mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}
static PT_RC_CHECK pt_pip_verify_config_block_crc_(u8 ebid, u8 *status, u16 *calculated_crc, u16 *stored_crc)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_VERIFY_CONFIG_BLOCK_CRC;
	u8 param[1] = {0};

	param[0] = ebid;
	pt_send_pip1_cmd(cmd_id, param, 1);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id)) {
			if (status)
				*status = cd.read_buf[5];
			if (calculated_crc)
				*calculated_crc = get_unaligned_le16(&cd.read_buf[6]);
			if (stored_crc)
				*stored_crc = get_unaligned_le16(&cd.read_buf[8]);
			return RC_PASS;
		} else
			PT_ERR("%s: Response mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_check_ic_crc_(void)
{
	PT_RC_CHECK rc;
	u8 status;
	u16 calculated_crc = 0;
	u16 stored_crc = 0;

	rc = pt_pip1_cmd_suspend_scan();
	if (rc)
		goto error;

	rc = pt_pip_verify_config_block_crc_(PT_TCH_PARM_EBID, &status, &calculated_crc, &stored_crc);
	if (rc)
		goto error_resume_scan;

	if (status) {
		rc = RC_FAIL;
		goto error_resume_scan;
	}

	PT_DBG("%s: CRC: calculated: 0x%04X, stored: 0x%04X\n", __func__, calculated_crc, stored_crc);
	pt_pip1_cmd_resume_scan();
	goto exit;

error_resume_scan:
	pt_pip1_cmd_resume_scan();
error:
	PT_ERR("%s: rc=%d\n", __func__, rc);
exit:
	return rc;
}

static PT_RC_CHECK pt_pip1_cmd_calibrate(u8 mode)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_CALIBRATE_IDACS;
	u8 param[1] = {0};

	param[0] = mode;
	pt_send_pip1_cmd(cmd_id, param, 1);
	int_state = pt_wait_and_check_cmd_id(1500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id)) {
			return RC_PASS;
		} else
			PT_ERR("%s: Response mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_pip1_cmd_init_baseline(u8 mode)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_INITIALIZE_BASELINES;
	u8 param[1] = {0};

	param[0] = mode;
	pt_send_pip1_cmd(cmd_id, param, 1);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id)) {
			return RC_PASS;
		} else
			PT_ERR("%s: Response mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

/* Calibration */
int pt_calibrate(void)
{
	PT_RC_CHECK rc;
	u8 mode = 0;

	rc = pt_pip1_cmd_suspend_scan();
	if (rc == RC_FAIL)
		goto resume_scan;

	for (mode = 0; mode < 3; mode++) {
		rc = pt_pip1_cmd_calibrate(mode);
		if (rc == RC_FAIL) {
			PT_ERR("%s: Calibrate %d mode fail\n", __func__, mode);
		}
	}

	rc = pt_pip1_cmd_init_baseline(0x07);

resume_scan:
	pt_pip1_cmd_resume_scan();
	return rc;
}

static PT_RC_CHECK pt_pip1_cmd_start_bl(void)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_START_BOOTLOADER;

	pt_send_pip1_cmd(cmd_id, NULL, 0);
	int_state = pt_wait_and_clear_int(500);

	if (int_state) {
		if ((cd.read_buf[0] == 0x00) && (cd.read_buf[1] == 0x00)) {
			PT_DBG("%s: Sentinel Recevided\n", __func__);
			return RC_PASS;
		}
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

static PT_RC_CHECK pt_enumeration(void)
{
	PT_RC_CHECK rc = RC_PASS;
	int try_cnt = 3;

	GPIO_SET_RST_MODE_OUTPUT();
	GPIO_SET_RST_HIGH();
	GPIO_SET_INT_MODE_INPUT();

retry:
	/* Reset device */
	pt_dut_reset();
	DELAY_MS(300);

	pt_flush_bus();
	cd.mode = pt_pip1_cmd_get_mode();
	if (cd.mode == PT_MODE_BOOTLOADER) {
		rc = pt_pip1_cmd_bl_launch();
		if (rc == RC_FAIL)
			PT_ERR("%s: Launch FW failure\n", __func__);
		if (try_cnt-- > 0)
			goto retry;
		else
			goto exit;
	}

	cd.mode = pt_pip1_cmd_get_mode();
	if (cd.mode != PT_MODE_OPERATIONAL) {
		PT_ERR("%s: Launch FW failure\n", __func__);
		rc = RC_FAIL;
		if (try_cnt-- > 0)
			goto retry;
		else
			goto exit;
	}

	rc = pt_pip1_cmd_get_sysinfo();
	if (rc == RC_FAIL) {
		PT_ERR("%s: Get sysinfo failure\n", __func__);
		if (try_cnt-- > 0)
			goto retry;
		else
			goto exit;
	}

	rc = pt_check_ic_crc_();
	if (rc == RC_FAIL)
		PT_ERR("%s: Config CRC failure\n", __func__);
exit:
	return rc;
}

static void pt_parse_touch_report(void)
{
	u8 report_id = 0;
	u8 report_reg = 0;
	u8 num_cur_tch = 0;
	u16 x = 0;
	u16 y = 0;
	u8 i = 0, touch_id = 0, event_id = 0;

	report_id = cd.read_buf[PIP1_RESP_REPORT_ID_OFFSET];
	if (0x01 == report_id) {
		report_reg = cd.read_buf[TOUCH_COUNT_BYTE_OFFSET];
		if (IS_LARGE_AREA(report_reg)) {
			PT_DBG("Large Object detected\n");
		}

		num_cur_tch = GET_NUM_TOUCHES(report_reg);
		if (num_cur_tch) {
			for (i = 0; i < num_cur_tch; i++) {
				touch_id = cd.read_buf[7 + i * 10 + 1] & 0x1f;
				event_id = (cd.read_buf[7 + i * 10 + 1] >> 5) & 0x03;
				x = cd.read_buf[7 + i * 10 + 2] | cd.read_buf[7 + i * 10 + 3] << 8;
				y = cd.read_buf[7 + i * 10 + 4] | cd.read_buf[7 + i * 10 + 5] << 8;
				if (event_id == 3)
					PT_DBG("(%d): Lift, ID=%d, X=%d, Y=%d\n", i, touch_id, x, y);
				else
					PT_DBG("(%d): Down, ID=%d, X=%d, Y=%d\n", i, touch_id, x, y);

				extern struct cyttsp5_data my_cyttsp5_data;
				if (my_cyttsp5_data.enable) {
					bool touched = (event_id == 3) ? 0 : 1;
					my_cyttsp5_data.callback(my_cyttsp5_data.dev, x, y, touched);
				}
			}
		} else {
			PT_DBG("All Lift-off\n");
		}
	}
}

static u32 pt_format_hex(u8 *input_buf, u32 input_len, char *out_buf, u32 out_buf_size)
{
	u32 input_index = 0;
	u32 output_index = 0;

	while (input_index < input_len) {

		if ((output_index + 3) > out_buf_size)
			break;

		sprintf(&out_buf[output_index], "%02X,", input_buf[input_index]);
		output_index += 3;
		input_index += 1;
	}

	if ((output_index + 1) > out_buf_size)
		sprintf(&out_buf[output_index - 1], "\n");
	else {
		sprintf(&out_buf[output_index], "\n");
		output_index++;
	}

	return output_index;
}
void pt_format_debug_info(void)
{
	u8 cmd_id = PIP1_CMD_ID_GET_NOISE_METRICS;
	u8 noise_metrix = 0;
	u8 report_id = 0;
	u8 event_id = 0;
	u8 touch_report = 0;
	u8 num_cur_tch = 0;
	u8 report_reg = 0;
	u8 static first_touch;
	char *out_buf = &mfg_print_buffer[0];
	u16 size = 0;
	int i = 0;

	if (cd.debug_level) {
		memset(out_buf, 0, MAX_BUF_LEN);
		size = get_unaligned_le16(&cd.read_buf[0]);
		report_id = cd.read_buf[PIP1_RESP_REPORT_ID_OFFSET];
		if (size >= 7 && (0x01 == report_id)) {
			touch_report = 1;
		} else if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id) && (report_id == 0x1f)) {
			noise_metrix = 1;
		}
		if (cd.debug_level == DEBUG_LEVEL_2 && touch_report) {
			report_reg = cd.read_buf[TOUCH_COUNT_BYTE_OFFSET];
			num_cur_tch = GET_NUM_TOUCHES(report_reg);
			for (i = 0; i < num_cur_tch; i++) {
				event_id = (cd.read_buf[7 + i * 10 + 1] >> 5) & 0x03;
				if (event_id == 3)
					break;
			}
			if (num_cur_tch && (first_touch == 0)) {
				first_touch = 1;
				pt_pip1_cmd_send_noise_metric();
			}
			if (num_cur_tch && (event_id == 3)) {
				first_touch = 0;
				pt_pip1_cmd_send_noise_metric();
			}
		}

		if (cd.debug_level == DEBUG_LEVEL_3 && touch_report) {
			pt_pip1_cmd_send_noise_metric();
		}

		if (noise_metrix || touch_report)
			pt_format_hex(cd.read_buf, size, out_buf, MAX_BUF_LEN);
	}

	PT_ERR("%d,%u,%s", cd.debug_level, GET_SYSTEM_TICK(), out_buf);
}

/*****************************************************
 *                FW Update
 * **************************************************/
#if defined(UPDATE_FW_BY_HEADER)
static PT_RC_CHECK pt_pip1_write_data_block_(u16 row_number, u16 write_length, u8 ebid, u8 *write_buf,
											 u16 *actual_write_len)
{
	int int_state;
	u8 cmd_id = PIP1_CMD_ID_WRITE_DATA_BLOCK;
	u16 full_write_length = write_length + 15;
	u8 full_write_buf[PT_DATA_ROW_SIZE + 15];
	u8 cmd_offset = 0;
	u16 crc;

	memset(&full_write_buf, 0, ARRAY_SIZE(full_write_buf));
	full_write_buf[cmd_offset++] = LOW_BYTE(row_number);
	full_write_buf[cmd_offset++] = HIGH_BYTE(row_number);
	full_write_buf[cmd_offset++] = LOW_BYTE(write_length);
	full_write_buf[cmd_offset++] = HIGH_BYTE(write_length);
	full_write_buf[cmd_offset++] = ebid;
	memcpy(&full_write_buf[cmd_offset], write_buf, write_length);
	cmd_offset += write_length;
	memcpy(&full_write_buf[cmd_offset], pt_data_block_security_key, 8);
	cmd_offset += 8;
	crc = _pt_compute_crc(&full_write_buf[5], write_length);
	full_write_buf[cmd_offset++] = LOW_BYTE(crc);
	full_write_buf[cmd_offset++] = HIGH_BYTE(crc);

	pt_send_pip1_cmd(cmd_id, full_write_buf, full_write_length);
	int_state = pt_wait_and_check_cmd_id(500, cmd_id);

	if (int_state) {
		if (CHECK_PIP1_RESP_ID_AND_STATUS(cd.read_buf, cmd_id)) {
			if (actual_write_len)
				*actual_write_len = get_unaligned_le16(&cd.read_buf[7]);
			return RC_PASS;
		} else
			PT_ERR("%s: Response mismatch\n", __func__);
	} else {
		PT_ERR("%s: Miss INT\n", __func__);
	}

	return RC_FAIL;
}

/* 0-Not update; 1-Should Update */
static int pt_check_config_version_by_header(void)
{
	u16 fw_ver_file, fw_ver_cur;
	u16 conf_ver_file, conf_ver_cur;
	u32 fw_revctrl_file, fw_revctrl_cur;

	fw_ver_file = get_unaligned_be16(&ttconfig_fw_ver[2]);
	fw_revctrl_file = get_unaligned_be32(&ttconfig_fw_ver[8]);

	fw_ver_cur = (cd.fw_ver_major << 8) + cd.fw_ver_minor;
	fw_revctrl_cur = cd.revctrl;

	conf_ver_file = get_unaligned_be16(&ttconfig_fw_ver[16]);
	conf_ver_cur = cd.fw_ver_conf;

	if (fw_ver_file != fw_ver_cur) {
		PT_DBG("%s: FW version mismatch, can't update config\n", __func__);
		return 0;
	}

	if (fw_revctrl_file != fw_revctrl_cur) {
		PT_DBG("%s: Rev Control Num mismatch, can't update config\n", __func__);
		return 0;
	}

	if ((cd.post_code & PT_POST_TT_CFG_CRC_MASK) == 0) {
		PT_DBG("%s: POST, CFG CRC fail, should update\n", __func__);
		return 1;
	}

	if (conf_ver_file > conf_ver_cur) {
		PT_DBG("%s: conf_ver_file(%d) > conf_ver_cur(%d), should update\n", __func__, conf_ver_file, conf_ver_cur);
		return 1;
	}

	return 0;
}

static int pt_check_fw_version_by_header(void)
{
	u16 fw_ver_file, fw_ver_cur;
	u32 fw_revctrl_file, fw_revctrl_cur;

	fw_ver_file = get_unaligned_be16(&cyttsp4_ver[2]);
	fw_revctrl_file = get_unaligned_be32(&cyttsp4_ver[8]);

	fw_ver_cur = (cd.fw_ver_major << 8) + cd.fw_ver_minor;
	fw_revctrl_cur = cd.revctrl;

	if (fw_ver_file > fw_ver_cur) {
		PT_DBG("%s: fw_ver_file(%d) > fw_ver_cur(%d), should update\n", __func__, fw_ver_file, fw_ver_cur);
		return 1;
	}

	if (fw_revctrl_file > fw_revctrl_cur) {
		PT_DBG("%s: fw_revctrl_file(%d) > fw_revctrl_cur(%d), should update\n", __func__, fw_revctrl_file,
			   fw_revctrl_cur);
		return 1;
	}

	return 0;
}

static PT_RC_CHECK pt_update_ttconfig_by_header(void)
{
	u8 ebid = PT_TCH_PARM_EBID;
	u16 row_size = PT_DATA_ROW_SIZE;
	u16 table_size;
	u16 row_count;
	u16 residue;
	u8 *row_buf;
	u8 verify_crc_status;
	u16 calculated_crc;
	u16 stored_crc;
	PT_RC_CHECK rc = RC_PASS;
	int i;

	table_size = ARRAY_SIZE(cyttsp4_param_regs);
	row_count = table_size / row_size;
	row_buf = (u8 *)&cyttsp4_param_regs[0];

	rc = pt_pip1_cmd_suspend_scan();
	if (rc == RC_FAIL)
		goto resume_scan;

	for (i = 0; i < row_count; i++) {
		PT_DBG("%s: row=%d size=%d\n", __func__, i, row_size);
		rc = pt_pip1_write_data_block_(i, row_size, ebid, row_buf, NULL);
		if (rc) {
			PT_ERR("%s: Fail put row=%d r=%d\n", __func__, i, rc);
			break;
		}
		row_buf += row_size;
	}

	if (!rc) {
		residue = table_size % row_size;
		PT_DBG("%s: Last row=%d size=%d\n", __func__, i, residue);
		rc = pt_pip1_write_data_block_(i, residue, ebid, row_buf, NULL);
		row_count++;
		if (rc)
			PT_ERR("%s: Fail put row=%d r=%d\n", __func__, i, rc);
	}

	rc = pt_pip_verify_config_block_crc_(ebid, &verify_crc_status, &calculated_crc, &stored_crc);
	if (rc || verify_crc_status)
		PT_ERR("%s: CRC Failed, ebid=%d, status=%d, scrc=%X ccrc=%X\n", __func__, ebid, verify_crc_status,
			   calculated_crc, stored_crc);
	else
		PT_DBG("%s: CRC PASS, ebid=%d, status=%d, scrc=%X ccrc=%X\n", __func__, ebid, verify_crc_status, calculated_crc,
			   stored_crc);

resume_scan:
	pt_pip1_cmd_resume_scan();
	return rc;
}
#endif

static PT_RC_CHECK pt_ldr_enter_(struct pt_dev_id *dev_id)
{
	PT_RC_CHECK rc;
	u8 return_data[8];
	u8 mode = PT_MODE_OPERATIONAL;

	dev_id->silicon_id = 0;
	dev_id->rev_id = 0;
	dev_id->bl_ver = 0;

	/* Reset to get to a known state, sleep to allow sentinel processing */
	pt_dut_reset();
	DELAY_MS(30);
	pt_flush_bus();

	mode = pt_pip1_cmd_get_mode();
	PT_DBG("%s: mode=%d\n", __func__, mode);

	if (mode == PT_MODE_UNKNOWN)
		return RC_FAIL;

	if (mode != PT_MODE_BOOTLOADER) {
		rc = pt_pip1_cmd_start_bl();
		PT_DBG("%s:start_bl rc=%d\n", __func__, rc);
		if (rc)
			return rc;
		mode = pt_pip1_cmd_get_mode();
		PT_DBG("%s: mode=%d\n", __func__, mode);

		if (mode != PT_MODE_BOOTLOADER)
			return RC_FAIL;
	}

	rc = pt_pip1_cmd_bl_get_bl_info(return_data, 8);
	PT_DBG("%s:get_bl_info rc=%d\n", __func__, rc);
	if (rc)
		return rc;

	dev_id->silicon_id = get_unaligned_le32(&return_data[0]);
	dev_id->rev_id = return_data[4];
	dev_id->bl_ver = return_data[5] + (return_data[6] << 8) + (return_data[7] << 16);

	return rc;
}

static u8 *pt_get_row_(u8 *row_buf, u8 *image_buf, int size)
{
	memcpy(row_buf, image_buf, size);
	return image_buf + size;
}

static PT_RC_CHECK pt_ldr_parse_row_(u8 *row_buf, struct pt_hex_image *row_image)
{
	PT_RC_CHECK rc = RC_PASS;

	row_image->array_id = row_buf[PT_ARRAY_ID_OFFSET];
	row_image->row_num = get_unaligned_be16(&row_buf[PT_ROW_NUM_OFFSET]);
	row_image->row_size = get_unaligned_be16(&row_buf[PT_ROW_SIZE_OFFSET]);

	if (row_image->row_size > ARRAY_SIZE(row_image->row_data)) {
		PT_ERR("%s: row data buffer overflow\n", __func__);
		rc = RC_FAIL;
		goto pt_ldr_parse_row_exit;
	}

	memcpy(row_image->row_data, &row_buf[PT_ROW_DATA_OFFSET], row_image->row_size);
pt_ldr_parse_row_exit:
	return rc;
}

static PT_RC_CHECK pt_ldr_init_(struct pt_hex_image *row_image)
{
	return pt_pip1_cmd_bl_initiate_bl(row_image->row_size, row_image->row_data);
}

static PT_RC_CHECK pt_ldr_prog_row_(struct pt_hex_image *row_image)
{
	u16 length = row_image->row_size + 3;
	u8 data[3 + PT_DATA_ROW_SIZE];
	u8 offset = 0;

	data[offset++] = row_image->array_id;
	data[offset++] = LOW_BYTE(row_image->row_num);
	data[offset++] = HIGH_BYTE(row_image->row_num);
	memcpy(data + 3, row_image->row_data, row_image->row_size);
	return pt_pip1_cmd_bl_prog_and_verify(data, length);
}

static PT_RC_CHECK pt_ldr_exit_(void)
{
	return pt_pip1_cmd_bl_launch();
}

static PT_RC_CHECK pt_ldr_verify_chksum_(void)
{
	u8 result;
	PT_RC_CHECK rc;

	rc = pt_pip1_cmd_bl_verify_app_integrity(&result);
	if (rc)
		return rc;

	/* fail */
	if (result == 0)
		return RC_FAIL;

	return RC_PASS;
}

static PT_RC_CHECK pt_load_app_(const u8 *fw, int fw_size)
{
	struct pt_dev_id dev_id;
	struct pt_hex_image row_image;
	u8 row_buf[PT_DATA_MAX_ROW_SIZE];
	size_t image_rec_size;
	size_t row_buf_size = PT_DATA_MAX_ROW_SIZE;
	int row_count = 0;
	u8 *p;
	u8 *last_row;
	PT_RC_CHECK rc = RC_PASS;
	PT_RC_CHECK rc_tmp = RC_PASS;

	image_rec_size = sizeof(struct pt_hex_image);
	if (fw_size % image_rec_size != 0) {
		PT_ERR("%s: Firmware image is misaligned\n", __func__);
		goto exit;
	}

	rc = pt_ldr_enter_(&dev_id);
	if (rc) {
		PT_ERR("%s: Error cannot start Loader (ret=%d)\n", __func__, rc);
		goto exit;
	}

	PT_DBG("%s: silicon id=%08X rev=%02X bl=%08X\n", __func__, dev_id.silicon_id, dev_id.rev_id, dev_id.bl_ver);

	cd.mode = PT_MODE_BOOTLOADER;

	/* get last row */
	last_row = (u8 *)fw + fw_size - image_rec_size;
	pt_get_row_(row_buf, last_row, image_rec_size);
	pt_ldr_parse_row_(row_buf, &row_image);

	rc = pt_ldr_init_(&row_image);
	if (rc) {
		PT_ERR("%s: Error cannot init Loader (ret=%d)\n", __func__, rc);
		goto exit;
	}

	PT_DBG("%s: Send BL Loader Blocks\n", __func__);

	p = (u8 *)fw;
	while (p < last_row) {
		/* Get row */
		row_count += 1;
		PT_DBG("%s: read row=%d\n", __func__, row_count);

		memset(row_buf, 0, row_buf_size);
		p = pt_get_row_(row_buf, p, image_rec_size);

		/* Parse row */
		rc = pt_ldr_parse_row_(row_buf, &row_image);
		PT_DBG("%s: array_id=%02X row_num=%04X(%d) row_size=%04X(%d)\n", __func__, row_image.array_id,
			   row_image.row_num, row_image.row_num, row_image.row_size, row_image.row_size);
		if (rc) {
			PT_ERR("%s: Parse Row Error (a=%d r=%d ret=%d\n", __func__, row_image.array_id, row_image.row_num, rc);
			goto exit;
		}

		/* program row */
		rc = pt_ldr_prog_row_(&row_image);
		if (rc) {
			PT_ERR("%s: Program Row Error (array=%d row=%d ret=%d)\n", __func__, row_image.array_id, row_image.row_num,
				   rc);
			goto exit;
		}
	}

	/* exit loader */
	PT_DBG("%s: Send BL Loader Terminate\n", __func__);

	rc = pt_ldr_exit_();
	if (rc) {
		PT_ERR("%s: Error on exit Loader (ret=%d)\n", __func__, rc);

		/* verify app checksum */
		rc_tmp = pt_ldr_verify_chksum_();
		if (rc_tmp) {
			PT_ERR("%s: ldr_verify_chksum fail r=%d\n", __func__, rc_tmp);
		} else
			PT_DBG("%s: APP Checksum Verified\n", __func__);
	}

exit:
	PT_DBG("%s: exit\n", __func__);
	return rc;
}

#if defined(UPDATE_FW_BY_HEADER)
static PT_RC_CHECK pt_update_firmware_by_header(void)
{
	int retry = 3;
	PT_RC_CHECK rc = RC_PASS;

	while (retry--) {
		rc = pt_load_app_(cyttsp4_img, ARRAY_SIZE(cyttsp4_img));
		if (rc < 0)
			PT_DBG("%s: Firmware update failed rc=%d, retry:%d\n", __func__, rc, retry);
		else
			break;
		DELAY_MS(20);
	}

	if (rc < 0) {
		PT_DBG("%s: Firmware update failed with error code %d\n", __func__, rc);
	} else {
		rc = pt_enumeration();
		if (rc == RC_PASS)
			pt_calibrate();
	}

	return rc;
}
#endif

#if 0 // defined(UPDATE_FW_BY_BIN)
static int pt_check_firmware_version_by_bin(void)
{
	u16 fw_config_ver_new;
	u32 fw_ver_new;
	u32 fw_revctrl_new;
	u16 fw_config_ver_cur;
	u32 fw_ver_cur;
	u32 fw_revctrl_cur;
	const u8 *data;

	data = &TB_Test[0];

	fw_ver_new = get_unaligned_be16(data + 3);
	/* 4 middle bytes are not used */
	fw_revctrl_new = get_unaligned_be32(data + 9);
	/* Offset 17,18 is the TT Config version*/
	fw_config_ver_new = get_unaligned_be16(data + 17);

	PT_DBG("%s: Built-in FW version=0x%04x rev=%d config=0x%04X\n", __func__,
		   fw_ver_new, fw_revctrl_new, fw_config_ver_new);

	fw_ver_cur = (cd.fw_ver_major << 8) + cd.fw_ver_minor;
	fw_revctrl_cur = cd.revctrl;
	fw_config_ver_cur = cd.fw_ver_conf;

	PT_DBG("%s: Cur FW version=0x%04x rev=%d config=0x%04X\n", __func__,
		   fw_ver_cur, fw_revctrl_cur, fw_config_ver_cur);

	if (fw_ver_new > fw_ver_cur) {
		PT_DBG("%s: fw_ver_new(%d) > fw_ver_cur(%d), should update\n", __func__,
			   fw_ver_new, fw_ver_cur);
		return 1;
	}

	if (fw_revctrl_new > fw_revctrl_cur) {
		PT_DBG("%s: fw_revctrl_new(%d) > fw_revctrl_cur(%d), should update\n",
			   __func__, fw_revctrl_new, fw_revctrl_cur);
		return 1;
	}

	if (fw_config_ver_new > fw_config_ver_cur) {
		PT_DBG("%s: fw_config_ver_new(%d) > conf_ver_cur(%d), should update\n",
			   __func__, fw_config_ver_new, fw_config_ver_cur);
		return 1;
	}

	return 0;
}

static PT_RC_CHECK pt_update_firmware_by_bin(void)
{
	int retry = 3;
	const u8 *data;
	u8 header_size = 0;
	u32 image_size;
	PT_RC_CHECK rc = RC_PASS;

	data = &TB_Test[0];
	header_size = data[0];
	image_size = ARRAY_SIZE(TB_Test);

	while (retry--) {
		rc = pt_load_app_(&data[header_size + 1],
						  image_size - (header_size + 1));
		if (rc < 0)
			PT_ERR("%s: Firmware update failed rc=%d, retry:%d\n", __func__, rc,
				   retry);
		else
			break;
		DELAY_MS(20);
	}

	if (rc < 0) {
		PT_ERR("%s: Firmware update failed with error code %d\n", __func__, rc);
	} else {
		rc = pt_enumeration();
		if (rc == RC_PASS)
			pt_calibrate();
	}

	return rc;
}
#endif

PT_RC_CHECK pt_update(u8 force_update)
{
	u8 should_update = 1;
	PT_RC_CHECK rc = RC_PASS;
#if defined(UPDATE_FW_BY_HEADER)
	should_update = pt_check_fw_version_by_header();
	if (should_update || force_update)
		rc = pt_update_firmware_by_header();

	should_update = pt_check_config_version_by_header();
	if (should_update || force_update)
		rc = pt_update_ttconfig_by_header();
#elif defined(UPDATE_FW_BY_BIN)
#if 0
	should_update = pt_check_firmware_version_by_bin();
	if (should_update || force_update)
		rc = pt_update_firmware_by_bin();
#endif
#endif
	return rc;
}

/****************************************************
 *                   Debug function
 ****************************************************/
static volatile u8 noise_wait_count;
void pt_pip1_cmd_send_noise_metric(void)
{
	u8 cmd_id = PIP1_CMD_ID_GET_NOISE_METRICS;
	pt_send_pip1_cmd(cmd_id, NULL, 0);
}

void pt_parse_noise_metric(void)
{
	u8 report_id = 0;
	u8 cmd_id = PIP1_CMD_ID_GET_NOISE_METRICS;
	PT_NOISE_METRIC *noise_metric;

	report_id = cd.read_buf[PIP1_RESP_REPORT_ID_OFFSET];

	if (CHECK_PIP1_RESP_ID(cd.read_buf, cmd_id) && (report_id == 0x1f)) {
		noise_metric = (PT_NOISE_METRIC *)&cd.read_buf[5];
		PT_DBG("water_level=%d, inject_noise=%d, "
			   "temperature=%d,touch_type=%d\n",
			   noise_metric->water_level, noise_metric->inject_noise, noise_metric->temperature,
			   noise_metric->touch_type);
	}
}

/* Interrupt handler */
int pt_parse_cmd_and_interrupt(void)
{
	u8 report_id = 0;
	int rc = 0;
	u16 size;

	rc = pt_read_default_nosize(cd.read_buf, PT_MAX_PIP_MSG_SIZE);
	if (!rc) {
		size = get_unaligned_le16(&cd.read_buf[0]);
		if (!size || (size == 2) || size > PT_MAX_PIP_MSG_SIZE) {
			return 0;
		}
		report_id = cd.read_buf[PIP1_RESP_REPORT_ID_OFFSET];
		if (size >= 7 && (0x01 == report_id)) {
			pt_parse_touch_report();
			noise_wait_count = 0;
		} else if (0x04 == report_id) {
			PT_DBG("%s: wakeup event\n", __func__);
			printk("wake up !");
		} else {
			pt_parse_noise_metric();
		}
		// pt_format_debug_info();
	} else {
		PT_ERR("%s: I2C error\n", __func__);
	}
	return 0;
}

int pt_poll_noise_metric(void)
{
	/* Polling function should be 60 Hz, and here use 4Hz to poll noise metric
	 */
	if ((cd.debug_level == DEBUG_LEVEL_3) && (noise_wait_count++ > 15)) {
		noise_wait_count = 0;
		pt_pip1_cmd_send_noise_metric();
	}
	return 0;
}


static K_SEM_DEFINE(touch_sem, 0, 1);

int pt_interrupt(void)
{
	k_sem_give(&touch_sem);
	return 0;
}

/****************************************************
 *                   User function
 ****************************************************/
/* Interrupt handler */
int pt_parse_interrupt(void)
{
	int rc;

	rc = pt_read_default_nosize(cd.read_buf, PT_MAX_PIP_MSG_SIZE);
	if (rc) {
		PT_ERR("read error!\n");
		return -1;
	}

	pt_parse_touch_report();

	return 0;
}

#if 1
static K_KERNEL_STACK_DEFINE(tp_stack, 1024);
static struct k_thread tp_thread;
static void touch_daemon_thread(void *p1, void *p2, void *p3)
{
	uint32_t event_out;
	int ret;

	for (;;) {
		k_sem_take(&touch_sem, K_FOREVER);
		pt_parse_cmd_and_interrupt();
	}
}
#endif

#include "zephyr.h"

/* Example - To perform all the functions */
void pt_example(void)
{
	PT_RC_CHECK rc = RC_PASS;
	u8 force_update = 0;

	DISABLE_IRQ();
	rc = pt_enumeration();
	if (rc == RC_PASS) {
		PT_DBG("%s: Enumeration is successful\n", __func__);
	} else {
		force_update = 1;
	}
#if 0
	rc = pt_update(force_update);
	if (rc == RC_PASS) {
		PT_DBG("%s: update is successful\n", __func__);
	}

	rc = pt_cmcp_test();
	if (rc == RC_PASS) {
		PT_DBG("%s: cmcp_test is successful\n", __func__);
	}
#endif

	cd.debug_level = DEBUG_LEVEL_1;
#if 0
	REGISTER_INT_HANDLER(pt_parse_cmd_and_interrupt);
#else
	REGISTER_INT_HANDLER(pt_interrupt);
#endif
//	REGISTER_60HZ_HANDLER(pt_poll_noise_metric); //no need
#if 1
	k_thread_create(&tp_thread, tp_stack, K_KERNEL_STACK_SIZEOF(tp_stack), 
					touch_daemon_thread, NULL, NULL, NULL,
					CONFIG_KSCAN_CYTTSP5_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&tp_thread, "/kscan@touch");

#endif
	ENABLE_IRQ();

	PT_DBG("%s: Exit\n", __func__);
}
