
#include <kernel.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
#include <soc.h>
#include <init.h>
#include <device.h>
#include <drivers/uart.h>

#include <linker/sections.h>
#include <logging/log.h>


#define DT_DRV_COMPAT ambiq_apllo3p_usart

//LOG_MODULE_REGISTER(apollo3p_uart);
#define CONFIG_UART_DEFAULT_BAUD 115200

/*
 * Contorl register
 */
#define UART_CR_UARTEN BIT(0)
#define UART_CR_CLKEN BIT(3)

#define UART_CR_CLKSEL(x) ((x & 0x7) << 4) //BSP_FLD32(x, 4, 6)
#define CLK_24MHZ 0x1
#define CLK_12HZ 0x2
#define CLK_6MHZ 0x3
#define CLK_3MHZ 0x4

#define UART_CR_TXE BIT(8)
#define UART_CR_RXE BIT(9)
#define UART_CR_RTSEN BIT(14)
#define UART_CR_CRSEN BIT(15)
#define UART_CR_FLOW_MASK (UART_CR_RTSEN | UART_CR_CRSEN)

/*
 * Line control register 
 */
#define UART_LCRH_BRK BIT(0)
#define UART_LCRH_PEN BIT(1)
#define UART_LCRH_EPS BIT(2)
#define UART_LCRH_STOP BIT(3)
#define UART_LCRH_FEN BIT(4)
#define UART_LCRH_WLEN(x) ((x & 0x3) << 5) //BSP_FLD32(x, 5, 6)
#define UART_LCRH_SPS BIT(7)

/*
 * FIFO interrupt level register
 */
#define UART_IFLS_TX_LEVEL(x) ((x & 0x7) << 0) //BSP_FLD32(x, 0, 2)
#define UART_IFLS_RX_LEVEL(x) ((x & 0x7) << 3) //BSP_FLD32(x, 3, 5)

/*
 * Interrupt enable register
 */
#define UART_IER_RX	BIT(4)
#define UART_IER_TX BIT(0)
#define UART_IER_RX_TIMEOUT BIT(6)

/*
 * Flag register
 */
#define UART_FR_RXFE BIT(4) /* RX fifo empty */
#define UART_FR_TXFF BIT(5) /* TX fifo full */
#define UART_FR_RXFF BIT(6) /* RX fifo full */
#define UART_FR_TXFE BIT(7) /* TX fifo empty */

#define MAX_HWFIFO_SIZE 32

struct uart_regs {
	uint32_t		dr; /* UART data register */
	uint32_t		rsr; /* UART status register */
	uint32_t		reserved_0[4];
	uint32_t		fr; /* Flag register */
	uint32_t		reserved_1[1];
	uint32_t		ilpr; /* IrDA counter */
	uint32_t		ibrd; /* Integer baud rate divisor */
	uint32_t		fbrd; /* Fractional baud rate divsor */
	uint32_t		lcrh; /* Line contrl high */
	uint32_t		cr; /* Control register */
	uint32_t		ifls; /* FIFO interrupt level select */
	uint32_t		ier; /* Interrupt enable */
	uint32_t		ies; /* Interrupt status */
	uint32_t		mis; /* Masked interrupt status */
	uint32_t		iec; /* Interrupt clear */
};

struct apollo_uart_config {
	volatile struct uart_regs *reg;
	int irq;
    int priority;
	uint32_t intmask;
	uint32_t pwren;
};

/* driver data */
struct apollo_uart {
    struct uart_config format;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    uart_irq_callback_user_data_t user_cb;
    void *user_data;
#endif
};

#define UART_ENABLED(n) \
        DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(uart##n), ambiq_apllo3p_usart, okay)

#define DEV_CFG(dev)							\
	((const struct apollo_uart_config *)(dev)->config)
#define UART_STRUCT(dev)					\
	((volatile struct uart_regs *)(DEV_CFG(dev))->reg)



static int apollo3p_uart_set_baudrate(volatile struct uart_regs *reg,
	int baudrate)
{
	uint32_t cr = reg->cr;
	uint32_t clk, div_h, div_l;
	uint64_t temp;

	if (baudrate > 921600)
		return -EINVAL;

	switch ((cr & 0x70) >> 4) {
	case CLK_24MHZ:
		clk = 24000000;
		break;
	case CLK_12HZ:
		clk = 12000000;
		break;
	case CLK_6MHZ:
		clk = 6000000;
		break;
	case CLK_3MHZ:
		clk = 3000000;
		break;
        default:
               return -EINVAL;
	}

	temp = 16 * baudrate;
	div_h = clk / (uint32_t)temp;
	temp = (uint64_t)clk * 64 / temp;
	div_l = (uint32_t)(temp - div_h * 64);

	reg->ibrd = div_h;
	reg->ibrd = div_h;
	reg->fbrd = div_l;
	return 0;
}

static int apollo3p_uart_poll_in(const struct device *dev, 
    unsigned char *c)
{
    volatile struct uart_regs *reg = UART_STRUCT(dev);

    if ((reg->fr & UART_FR_RXFE))
        return -1;

    *c = reg->dr & 0xFF;
	return 0;
}

static void apollo3p_uart_poll_out(const struct device *dev,
    unsigned char c)
{
    volatile struct uart_regs *reg = UART_STRUCT(dev);

    while (reg->fr & UART_FR_TXFF);
    reg->dr = c;
}

static int apollo3p_uart_err_check(const struct device *dev)
{
	return 0;
}

static int apollo3p_uart_configure(const struct device *dev,
    const struct uart_config *cfg)
{
    volatile struct uart_regs *reg = UART_STRUCT(dev);
	struct apollo_uart *data = dev->data;
    uint32_t cr;
	uint32_t lcr;

	if (cfg->parity == UART_CFG_PARITY_MARK ||
	    cfg->parity == UART_CFG_PARITY_SPACE)
		return -ENOTSUP;
    
    if (cfg->stop_bits == UART_CFG_STOP_BITS_0_5 ||
        cfg->stop_bits == UART_CFG_STOP_BITS_1_5)
        return -ENOTSUP;
    
    if (cfg->data_bits == UART_CFG_DATA_BITS_9)
        return -ENOTSUP;

    if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE)
        return -ENOTSUP;

    lcr = 0;
	cr = reg->cr;
	reg->cr = cr & ~(UART_CR_UARTEN | UART_CR_FLOW_MASK);

    if (data->format.data_bits != cfg->data_bits) {
        switch (cfg->data_bits) {
        case UART_CFG_DATA_BITS_5:
            lcr |= UART_LCRH_WLEN(0);
            break;
        case UART_CFG_DATA_BITS_6:
            lcr |= UART_LCRH_WLEN(1);
            break;
        case UART_CFG_DATA_BITS_7:
            lcr |= UART_LCRH_WLEN(2);
            break;
        case UART_CFG_DATA_BITS_8:
            lcr |= UART_LCRH_WLEN(3);
            break;
        default:
            return -ENOTSUP;
        }

        data->format.data_bits = cfg->data_bits;
    }

    if (data->format.stop_bits != cfg->stop_bits) {
        if (cfg->stop_bits == UART_CFG_STOP_BITS_2)
            lcr |= UART_LCRH_STOP;
        data->format.stop_bits = cfg->stop_bits;
    }

    if (data->format.parity != cfg->parity) {
        if (cfg->parity) {
            if (cfg->parity == UART_CFG_PARITY_ODD)
                lcr |= UART_LCRH_PEN;
            else
                lcr |= UART_LCRH_PEN | UART_LCRH_EPS;
        }

        data->format.parity = cfg->parity;
    }

	if (data->format.baudrate != cfg->baudrate) {
        apollo3p_uart_set_baudrate(reg, cfg->baudrate);
		data->format.baudrate = cfg->baudrate;
	}

    reg->lcrh = lcr;
    reg->cr = cr;
	return 0;
};

static int apollo3p_uart_config_get(const struct device *dev,
    struct uart_config *cfg)
{
	struct apollo_uart *data = dev->data;
    *cfg = data->format;
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int apollo3p_uart_fifo_fill(const struct device *dev,
    const uint8_t *buf, int size)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);
        int n = 0;

	while (n < size) {
		if (reg->fr & UART_FR_TXFF)
			break;
		reg->dr = buf[n++];
	}
	return n;
}

static int apollo3p_uart_fifo_read(const struct device *dev, 
    uint8_t *buf, const int size)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);
    int n = 0;

    while (n < size) {
        if (reg->fr & UART_FR_RXFE)
            break;
        buf[n++] = reg->dr & 0xFF;
    }
    return n;
}

static void apollo3p_uart_irq_tx_enable(const struct device *dev)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);

	reg->ier |= UART_IER_TX;
}

static void apollo3p_uart_irq_tx_disable(const struct device *dev)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);

	reg->ier &= ~UART_IER_TX;
}

static int apollo3p_uart_irq_tx_ready(const struct device *dev)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);

	return !(reg->fr & UART_FR_TXFF);
}

static int apollo3p_uart_irq_tx_complete(const struct device *dev)
{
    volatile struct uart_regs *reg = UART_STRUCT(dev);

    return !!(reg->fr & UART_FR_TXFE);
}

static void apollo3p_uart_irq_rx_enable(const struct device *dev)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);

	reg->ier |= UART_IER_RX | UART_IER_RX_TIMEOUT;
}

static void apollo3p_uart_irq_rx_disable(const struct device *dev)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);

	reg->ier &= ~(UART_IER_RX | UART_IER_RX_TIMEOUT);
}

static int apollo3p_uart_irq_rx_ready(const struct device *dev)
{
	volatile struct uart_regs *reg = UART_STRUCT(dev);

	return !(reg->fr & UART_FR_RXFE);
}

static void apollo3p_uart_irq_err_enable(const struct device *dev)
{
    (void) dev;
}

static void apollo3p_uart_irq_err_disable(const struct device *dev)
{
    (void) dev;
}

static int apollo3p_uart_irq_is_pending(const struct device *dev)
{
    volatile struct uart_regs *reg = UART_STRUCT(dev);
    const struct apollo_uart_config *cfg = DEV_CFG(dev);

     return !!(reg->ies & cfg->intmask);
}

static int apollo3p_uart_irq_update(const struct device *dev)
{
	return 1;
}

static void apollo3p_uart_irq_callback_set(const struct device *dev,
    uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct apollo_uart *data = dev->data;

	data->user_cb = cb;
	data->user_data = cb_data;
}

static void apollo3p_uart_isr(const void *dev)
{
	struct apollo_uart *data = ((const struct device *)dev)->data;

	if (data->user_cb) 
		data->user_cb(dev, data->user_data);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int apollo3p_uart_init(const struct device *dev)
{
    volatile struct uart_regs *reg = UART_STRUCT(dev);
    const struct apollo_uart_config *cfg = DEV_CFG(dev);

    /* Enable UART clock */
    PWRCTRL->DEVPWREN |= cfg->pwren;
    k_busy_wait(2);

    reg->cr = UART_CR_CLKSEL(CLK_24MHZ);
    reg->cr |= UART_CR_CLKEN;
	
    apollo3p_uart_set_baudrate(reg, CONFIG_UART_DEFAULT_BAUD);
    reg->lcrh = UART_LCRH_WLEN(3);

    /* Configure FIFO */
    reg->ifls = UART_IFLS_TX_LEVEL(2) | UART_IFLS_RX_LEVEL(2);
    reg->lcrh |= UART_LCRH_FEN;

    /* Configure interrupt */
    reg->iec = 0x7FF;
    reg->ier = 0;
	
    /* Enable UART */
    reg->cr |= UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
    	
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    irq_connect_dynamic(cfg->irq, cfg->priority, 
        apollo3p_uart_isr, dev, 0);
    irq_enable(cfg->irq);
#endif
    return 0;    
}

#if UART_ENABLED(0) || UART_ENABLED(1)
static const struct uart_driver_api apollo3p_uart_driver_api = {
	.poll_in = apollo3p_uart_poll_in,
	.poll_out = apollo3p_uart_poll_out,
	.err_check = apollo3p_uart_err_check,
	.configure = apollo3p_uart_configure,
	.config_get = apollo3p_uart_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = apollo3p_uart_fifo_fill,
	.fifo_read = apollo3p_uart_fifo_read,
	.irq_tx_enable = apollo3p_uart_irq_tx_enable,
	.irq_tx_disable = apollo3p_uart_irq_tx_disable,
	.irq_tx_ready = apollo3p_uart_irq_tx_ready,
	.irq_tx_complete = apollo3p_uart_irq_tx_complete,
	.irq_rx_enable = apollo3p_uart_irq_rx_enable,
	.irq_rx_disable = apollo3p_uart_irq_rx_disable,
	.irq_rx_ready = apollo3p_uart_irq_rx_ready,
	.irq_err_enable = apollo3p_uart_irq_err_enable,
	.irq_err_disable = apollo3p_uart_irq_err_disable,
	.irq_is_pending = apollo3p_uart_irq_is_pending,
	.irq_update = apollo3p_uart_irq_update,
	.irq_callback_set = apollo3p_uart_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
};
#endif /* CONFIG_APOLLO3P_UART0 || CONFIG_APOLLO3P_UART1 */

#define UART_DEVICE(nr) \
static struct apollo_uart apollo3p_uart##nr##_private;  \
static const struct apollo_uart_config apollo3p_uart##nr##_config = { \
    .reg = (volatile struct uart_regs *)UART##nr##_BASE, \
    .irq = UART##nr##_IRQn,                           \
    .priority = DT_IRQ(DT_NODELABEL(uart##nr), priority),  \
    .intmask = UART_IER_RX | UART_IER_RX_TIMEOUT | UART_IER_TX, \
    .pwren = BIT(nr+7), \
};                              \
DEVICE_DT_DEFINE(DT_DRV_INST(nr),   \
	apollo3p_uart_init,       \
	NULL, \
	&apollo3p_uart##nr##_private,  \
	&apollo3p_uart##nr##_config,   \
	PRE_KERNEL_1,          \
	CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
	&apollo3p_uart_driver_api);


#if UART_ENABLED(0)
UART_DEVICE(0)
#endif
#if UART_ENABLED(1)
UART_DEVICE(1)
#endif