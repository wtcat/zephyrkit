#ifndef TIMER_CLOCK_H_
#define TIMER_CLOCK_H_

#define CTIMER_CTRL0_TMRA0CLK(n) ((n << 1) & 0x0000003E)

#define CLK_PIN               CTIMER_CTRL0_TMRA0CLK(0x0)
#define HFRC_24MHZ            CTIMER_CTRL0_TMRA0CLK(0x1)
#define HFRC_3MHZ             CTIMER_CTRL0_TMRA0CLK(0x2)
#define HFRC_187_5KHZ         CTIMER_CTRL0_TMRA0CLK(0x3)
#define HFRC_47KHZ            CTIMER_CTRL0_TMRA0CLK(0x4)
#define HFRC_12KHZ            CTIMER_CTRL0_TMRA0CLK(0x5)
#define XT_32_768KHZ          CTIMER_CTRL0_TMRA0CLK(0x6)
#define XT_16_384KHZ          CTIMER_CTRL0_TMRA0CLK(0x7)
#define XT_2_048KHZ           CTIMER_CTRL0_TMRA0CLK(0x8)
#define XT_256HZ              CTIMER_CTRL0_TMRA0CLK(0x9)
#define LFRC_512HZ            CTIMER_CTRL0_TMRA0CLK(0xA)
#define LFRC_32HZ             CTIMER_CTRL0_TMRA0CLK(0xB)
#define LFRC_1HZ              CTIMER_CTRL0_TMRA0CLK(0xC)
#define LFRC_1_16HZ           CTIMER_CTRL0_TMRA0CLK(0xD)
#define RTC_100HZ             CTIMER_CTRL0_TMRA0CLK(0xE)
#define HCLK                  CTIMER_CTRL0_TMRA0CLK(0xF)

#endif /* TIMER_CLOCK_H_ */
