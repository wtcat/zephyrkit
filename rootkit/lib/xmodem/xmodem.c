/*
 * Copyright 2001-2021 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* this code needs standard functions memcpy() and memset()
   and input/output functions _inbyte() and _outbyte().

   the prototypes of the input/output functions are:
	 int _inbyte(unsigned short timeout); // msec timeout
	 void _outbyte(int c);

 */

#include "crc16.h"
#include "xmodem_porting.h"
#include "stdio.h"
#include "string.h"
#include "zephyr.h"
#include "storage/flash_map.h"
#include <shell/shell.h>
#include "stdlib.h"

#define USE_BLOCK_ERASE

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25
#define ERASE_BLKSZ (256 * 1024)


typedef enum {
	TRANS_TYPE_PARTITION = 0,
	TRANS_TYPE_FILE = 1,
} trans_type;

typedef struct {
	uint8_t id;
	uint32_t size;
} trans_info_partition;

typedef struct {
	char *filename;
	uint32_t size;
} trans_info_file;

typedef union {
	trans_info_partition partition_info;
	trans_info_file file_info;
} trans_info;

typedef struct {
	struct k_thread thread;
	trans_type type;
	trans_info info;
} trans_desc;

static int check(int crc, const unsigned char *buf, int sz)
{
	if (crc) {
		unsigned short crc = crc16_ccitt_custom(buf, sz);
		unsigned short tcrc = (buf[sz] << 8) + buf[sz + 1];
		if (crc == tcrc)
			return 1;
	} else {
		int i;
		unsigned char cks = 0;
		for (i = 0; i < sz; ++i) {
			cks += buf[i];
		}
		if (cks == buf[sz])
			return 1;
	}

	return 0;
}

static void flushinput(void)
{
	while (_inbyte(((DLY_1S)*3) >> 1) >= 0)
		;
}
void uart_irq_configure(bool enable);

static uint8_t need_erase = 0;
int xmodemReceive(trans_desc *desc)
{
	if (desc == NULL) {
		return -5;
	}
	int destsz = 0;
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	unsigned char *p;
	int bufsz, crc = 0;
	unsigned char trychar = 'C';
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry, retrans = MAXRETRANS;
	const struct flash_area *dest_area;
	trans_type type = desc->type;
	uint32_t erase_ofs = 0;

	switch (type) {
	case TRANS_TYPE_PARTITION: {
		if (flash_area_open(desc->info.partition_info.id, &dest_area)) {
			return -5; // open fail!
		}
		// erase all first!
		destsz = dest_area->fa_size;
		need_erase = 1;
	} break;
	default:
		printk("error, return!\n");
		return -5; // not suppot.
	}

	for (;;) {
		for (retry = 0; retry < 16; ++retry) {
			if (trychar)
				_outbyte(trychar);
			if ((c = _inbyte((DLY_1S) << 1)) >= 0) {
				switch (c) {
				case SOH:
					bufsz = 128;
					goto start_recv;
				case STX:
					bufsz = 1024;
					goto start_recv;
				case EOT:
					flushinput();
					_outbyte(ACK);
					return len; /* normal end */
				case CAN:
					if ((c = _inbyte(DLY_1S)) == CAN) {
						flushinput();
						_outbyte(ACK);
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		if (trychar == 'C') {
			trychar = NAK;
			continue;
		}
		flushinput();
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		return -2; /* sync error */

	start_recv:
		if (trychar == 'C')
			crc = 1;
		trychar = 0;
		p = xbuff;
		*p++ = c;
		for (i = 0; i < (bufsz + (crc ? 1 : 0) + 3); ++i) {
			if ((c = _inbyte(DLY_1S)) < 0)
				goto reject;
			*p++ = c;
		}

		if (xbuff[1] == (unsigned char)(~xbuff[2]) &&
			(xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno - 1) && check(crc, &xbuff[3], bufsz)) {
			if (xbuff[1] == packetno) {
				register int count = destsz - len;
				if (count > bufsz)
					count = bufsz;
				if (count > 0) {
					switch (type) {
					case TRANS_TYPE_PARTITION: {
					#ifndef USE_BLOCK_ERASE
						if (1 == need_erase) {
							flash_area_erase(dest_area, 0, dest_area->fa_size);
							need_erase = 0;
						}
					#else /* USE_BLOCK_ERASE */
						if (len + count > erase_ofs) {
							if (!flash_area_erase(dest_area, erase_ofs, ERASE_BLKSZ)) {
								erase_ofs += ERASE_BLKSZ;
							} else {
								//TODO: Show error information
								goto reject;
							}
						}
					#endif /* USE_BLOCK_ERASE */
						flash_area_write(dest_area, len, &xbuff[3], count);
					} break;
					default: // not support,do nothing
						break;
					}
					len += count;
				}
				++packetno;
				retrans = MAXRETRANS + 1;
			}
			if (--retrans <= 0) {
				flushinput();
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				return -3; /* too many retry error */
			}
			_outbyte(ACK);
			continue;
		}
	reject:
		flushinput();
		_outbyte(NAK);
	}
}

int xmodemTransmit(unsigned char *src, int srcsz)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	int bufsz, crc = -1;
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry;

	for (;;) {
		for (retry = 0; retry < 16; ++retry) {
			if ((c = _inbyte((DLY_1S) << 1)) >= 0) {
				switch (c) {
				case 'C':
					crc = 1;
					goto start_trans;
				case NAK:
					crc = 0;
					goto start_trans;
				case CAN:
					if ((c = _inbyte(DLY_1S)) == CAN) {
						_outbyte(ACK);
						flushinput();
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		flushinput();
		return -2; /* no sync */

		for (;;) {
		start_trans:
			xbuff[0] = SOH;
			bufsz = 128;
			xbuff[1] = packetno;
			xbuff[2] = ~packetno;
			c = srcsz - len;
			if (c > bufsz)
				c = bufsz;
			if (c >= 0) {
				memset(&xbuff[3], 0, bufsz);
				if (c == 0) {
					xbuff[3] = CTRLZ;
				} else {
					memcpy(&xbuff[3], &src[len], c);
					if (c < bufsz)
						xbuff[3 + c] = CTRLZ;
				}
				if (crc) {
					unsigned short ccrc = crc16_ccitt_custom(&xbuff[3], bufsz);
					xbuff[bufsz + 3] = (ccrc >> 8) & 0xFF;
					xbuff[bufsz + 4] = ccrc & 0xFF;
				} else {
					unsigned char ccks = 0;
					for (i = 3; i < bufsz + 3; ++i) {
						ccks += xbuff[i];
					}
					xbuff[bufsz + 3] = ccks;
				}
				for (retry = 0; retry < MAXRETRANS; ++retry) {
					for (i = 0; i < bufsz + 4 + (crc ? 1 : 0); ++i) {
						_outbyte(xbuff[i]);
					}
					if ((c = _inbyte(DLY_1S)) >= 0) {
						switch (c) {
						case ACK:
							++packetno;
							len += bufsz;
							goto start_trans;
						case CAN:
							if ((c = _inbyte(DLY_1S)) == CAN) {
								_outbyte(ACK);
								flushinput();
								return -1; /* canceled by remote */
							}
							break;
						case NAK:
						default:
							break;
						}
					}
				}
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				flushinput();
				return -4; /* xmit error */
			} else {
				for (retry = 0; retry < 10; ++retry) {
					_outbyte(EOT);
					if ((c = _inbyte((DLY_1S) << 1)) == ACK)
						break;
				}
				flushinput();
				return (c == ACK) ? len : -5;
			}
		}
	}
}

static int cmd_xmodem_rx(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, "argument num error!");
		shell_print(shell, "xmodem rx partiiton [id]");
		shell_print(shell, "use <flash_map list> get partition id");
		return 0;
	}

	trans_desc desc;
	if (0 == strcmp(argv[1], "partition")) {
		shell_print(shell, "*******need wait partition erase ok, so first packet will wait a long time*******");

		desc.type = TRANS_TYPE_PARTITION;
		desc.info.partition_info.id = atoi(argv[2]);
		shell_print(shell, "id = %d", desc.info.partition_info.id);

		int backend_count;

		backend_count = log_backend_count_get();

		for (int i = 0; i < backend_count; i++) {
			const struct log_backend *backend = log_backend_get(i);

			if (0 == strcmp(backend->name, "shell_uart_backend")) {
				log_backend_deactivate(backend);
				break;
			}
		}

		uart_irq_configure(false);
		unsigned int key = irq_lock();
		xmodemReceive(&desc);
		irq_unlock(key);
		uart_irq_configure(true);

		for (int i = 0; i < backend_count; i++) {
			const struct log_backend *backend = log_backend_get(i);

			if (0 == strcmp(backend->name, "shell_uart_backend")) {
				log_backend_activate(backend, backend->cb->ctx);
				break;
			}
		}

	} else {
		shell_print(shell, "now only support partition rx");
		return 0;
	}

	return 0;
}

static int cmd_xmodem_tx(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "not support!");
	return 0;
}

static void sub_cmd_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx == 0) {
		entry->syntax = "partition";
	} else if (idx == 1) {
		entry->syntax = "file";
	} else {
		entry->syntax = NULL;
	}
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_cmd, sub_cmd_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_xmodem, SHELL_CMD(rx, &dsub_cmd, "xmodem rx", cmd_xmodem_rx), // trans cmd
							   SHELL_CMD(tx, NULL, "xmodem tx", cmd_xmodem_tx),					 // trans cmd
							   SHELL_SUBCMD_SET_END, );

SHELL_CMD_REGISTER(xmodem, &sub_xmodem, "xmodem partition commands", NULL);
