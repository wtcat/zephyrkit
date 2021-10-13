#include "zephyr.h"
#include "drivers/uart.h"
// uart_poll_in();
static const struct device *uart_dev = NULL;

void _outbyte(int c)
{
	if (uart_dev != NULL) {
#if 0
		unsigned char ch;
		while (0 == uart_poll_in(uart_dev, &ch)) {
		}
#endif
		uart_poll_out(uart_dev, c);
	}
}

#if 0
void outbyte(int c)
{
	static char prev = 0;
	if (c < ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\b')
		return;
	if (c == '\n' && prev != '\r')
		_outbyte('\r');
	_outbyte(c);
	prev = c;
}
#endif

int _inbyte(unsigned short timeout) // msec timeout
{
	if (uart_dev != NULL) {

		unsigned char c;
		unsigned short delay = timeout;
		while (uart_poll_in(uart_dev, &c)) {
			k_busy_wait(1000);
			if (--delay == 0) {
				return -2;
			}
		}
		return c;
	} else {
		return -2;
	}
}

void uart_irq_configure(bool enable)
{
	if (enable) {
		uart_irq_rx_enable(uart_dev);
		uart_irq_tx_enable(uart_dev);
	} else {
		uart_irq_tx_disable(uart_dev);
		uart_irq_rx_disable(uart_dev);
	}
}
static int xmodem_init(const struct device *dev __unused)
{
	uart_dev = device_get_binding("UART_0");
	return 0;
}

SYS_INIT(xmodem_init, APPLICATION, 0);