#ifndef __XMODEM_PORTING_H__
#define __XMODEM_PORTING_H__

extern void _outbyte(int c);
extern int _inbyte(unsigned short timeout);
#endif