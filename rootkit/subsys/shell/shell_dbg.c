/*
 * Copyright (c) 2020 Actions Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <string.h>
#include <stdlib.h>
#include <device.h>
#include <soc.h>

#include <shell/shell.h>
#include <sys/printk.h>
#include <fs/fs.h>


#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
void print_buffer(const struct shell *shell,const char *addr, int width,
					int count, int linelen, unsigned long disp_addr)
{
	int i, thislinelen;
	const char *data;
	/* linebuf as a union causes proper alignment */
	union linebuf {
		uint32_t ui[MAX_LINE_LENGTH_BYTES/sizeof(uint32_t) + 1];
		uint16_t us[MAX_LINE_LENGTH_BYTES/sizeof(uint16_t) + 1];
		uint8_t  uc[MAX_LINE_LENGTH_BYTES/sizeof(uint8_t) + 1];
	} lb;

	if (linelen * width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	if (disp_addr == -1)
		disp_addr = (unsigned long)addr;

	while (count) {
		thislinelen = linelen;
		data = (const char *)addr;

		shell_fprintf(shell, SHELL_NORMAL, "%08x:", (unsigned int)disp_addr);

		/* check for overflow condition */
		if (count < thislinelen)
			thislinelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < thislinelen; i++) {
			if (width == 4) {
				lb.ui[i] = *(volatile uint32_t *)data;
				shell_fprintf(shell, SHELL_NORMAL, " %08x", lb.ui[i]);
			} else if (width == 2) {
				lb.us[i] = *(volatile uint16_t *)data;
				shell_fprintf(shell, SHELL_NORMAL, " %04x", lb.us[i]);
			} else {
				lb.uc[i] = *(volatile uint8_t *)data;
				shell_fprintf(shell, SHELL_NORMAL, " %02x", lb.uc[i]);
			}
			data += width;
		}

		while (thislinelen < linelen) {
			/* fill line with whitespace for nice ASCII print */
			for (i = 0; i < width * 2 + 1; i++)
				shell_fprintf(shell, SHELL_NORMAL, " ");
			linelen--;
		}

		/* Print data in ASCII characters */
		for (i = 0; i < thislinelen * width; i++) {
			if (lb.uc[i] < 0x20 || lb.uc[i] > 0x7e)
				lb.uc[i] = '.';
		}
		lb.uc[i] = '\0';
		shell_fprintf(shell, SHELL_NORMAL, "    %s\n", lb.uc);

		/* update references */
		addr += thislinelen * width;
		disp_addr += thislinelen * width;
		count -= thislinelen;
	}
}

#if defined(CONFIG_CMD_MEMORY)
#define DISP_LINE_LEN	16
static int do_mem_mw(const struct shell *shell, int width, size_t argc, char **argv)
{
	unsigned long writeval;
	unsigned long addr, count;
	char *buf;

	if (argc < 3)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);
	writeval = strtoul(argv[2], NULL, 16);

	if (argc == 4)
		count = strtoul(argv[3], NULL, 16);
	else
		count = 1;

	buf = (char *)addr;
	while (count-- > 0) {
		if (width == 4)
			*((uint32_t *)buf) = (uint32_t)writeval;
		else if (width == 2)
			*((uint16_t *)buf) = (uint16_t)writeval;
		else
			*((uint8_t *)buf) = (uint8_t)writeval;
		buf += width;
	}

	return 0;
}

static int do_mem_md(const struct shell *shell, int width, size_t argc, char **argv)
{
	unsigned long addr;
	int count;

	if (argc < 2)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);

	if (argc == 3)
		count = strtoul(argv[2], NULL, 16);
	else
		count = 1;

	print_buffer(shell, (char *)addr, width, count, DISP_LINE_LEN / width, -1);

	return 0;
}

static int shell_cmd_mdw(const struct shell *shell, size_t argc, char **argv)
{
	return do_mem_md(shell, 4, argc, argv);
}

static int shell_cmd_mdh(const struct shell *shell, size_t argc, char **argv)
{
	return do_mem_md(shell,2, argc, argv);
}

static int shell_cmd_mdb(const struct shell *shell, size_t argc, char **argv)
{
	return do_mem_md(shell,1, argc, argv);
}

static int shell_cmd_mww(const struct shell *shell, size_t argc, char **argv)
{
	return do_mem_mw(shell,4, argc, argv);
}

static int shell_cmd_mwh(const struct shell *shell, size_t argc, char **argv)
{
	return do_mem_mw(shell, 2, argc, argv);
}

static int shell_cmd_mwb(const struct shell *shell, size_t argc, char **argv)
{
	return do_mem_mw(shell, 1, argc, argv);
}
#endif	/* CONFIG_CMD_MEMORY */

static int print_buf(const struct shell *shell, char *ptr, int len)
{
	int i;
	shell_print(shell,"\n");
	for(i = 0; i < len; i++)
	{
		if(i % 16 == 0)
			shell_print(shell, "%8x: ", i);
		shell_print(shell, "%2x ", *ptr++);
		if(i % 16 == 15)
			shell_print(shell, "\n");
	}
	shell_print(shell, "\n");
	return 0;
}

#if defined(CONFIG_CMD_SPINOR)
#include <drivers/flash.h>
#define READ_BLOCK_ONCE  512

static int test_read_speed(const struct shell *shell, const struct device *nor_dev, 
    uint32_t  offset,  uint32_t size)
{
	uint64_t  start_ms;
	uint32_t  k, btn_ms;
	int ret;
	char *buf = k_malloc(READ_BLOCK_ONCE);
	if(buf == NULL){
		shell_print(shell, "malloc fail\n");
		return -1;
	}

	shell_print(shell, "nor read speed size=%d kb, offset=0x%x\n", size/1024, offset);
	//flash_read(nor_dev, 0x0, buf, READ_BLOCK_ONCE);
	//print_buf(buf);
	start_ms = k_uptime_get();
	for(k = 0; k < size; k += READ_BLOCK_ONCE, offset+=READ_BLOCK_ONCE) {
		 ret = flash_read((const struct device *)nor_dev, offset, buf, READ_BLOCK_ONCE);
		 if(ret < 0 ) {
			shell_print(shell, "flash read fail\n");
			break;
		 }
	}
	btn_ms = k_uptime_get() - start_ms;
	if(btn_ms == 0)
		btn_ms = 1;
	k_free(buf);
	shell_print(shell, "use %d ms, read once=%d,  nor speed =%d kb/s\n", btn_ms, READ_BLOCK_ONCE, (size/1024)*1000/btn_ms );

	return 0;
}

static int test_write_speed(const struct shell *shell, const struct device *nor_dev, 
    uint32_t  offset,  uint32_t size)
{
	uint64_t  start_ms;
	uint32_t  k, btn_ms, off, i;
	int ret;

	uint32_t *buf = k_malloc(READ_BLOCK_ONCE);
	if(buf == NULL){
		shell_print(shell, "malloc fail\n");
		return -1;
	}

	for(k = 0; k < READ_BLOCK_ONCE/4; k++)
		buf[k] = k;
	shell_print(shell,"nor write speed test, size=%d kb, offset=0x%x\n", size/1024, offset);
	off = offset;
#if 1
	start_ms = k_uptime_get();
	ret =  flash_erase(nor_dev, offset, size);
	 if(ret < 0 ) {
		shell_print(shell,"flash erase fail\n");
	 }
	btn_ms = k_uptime_get() - start_ms;
	if(btn_ms == 0)
		btn_ms = 1;
	shell_print(shell, "erase use %d ms, erase speed=%d kb/s\n", btn_ms,  (size/1024)*1000/btn_ms);

#endif
	start_ms = k_uptime_get();
	for(k = 0; k < size; k += READ_BLOCK_ONCE, offset+=READ_BLOCK_ONCE) {
		 ret = flash_write(nor_dev, offset, buf, READ_BLOCK_ONCE);
		 if(ret < 0 ) {
			shell_print(shell,"flash write fail\n");
			break;
		 }
	}
	btn_ms = k_uptime_get() - start_ms;
	if(btn_ms == 0)
		btn_ms = 1;
	shell_print(shell, "use %d ms, write once=%d,  nor speed =%d kb/s\n", btn_ms, READ_BLOCK_ONCE, (size/1024)*1000/btn_ms );

	shell_print(shell,"--cmp write and read---\n");
	for(k = 0; k < size; k += READ_BLOCK_ONCE, off+=READ_BLOCK_ONCE) {
		 ret = flash_read(nor_dev, off, buf, READ_BLOCK_ONCE);
		 if(ret < 0 ) {
			shell_print(shell,"flash read fail\n");
			break;
		 }
		 for(i = 0; i < READ_BLOCK_ONCE/4; i++) {
			if(buf[i] != i){
				shell_print(shell,"cmp fail: off=0x%x,val=0x%x != 0x%x\n", (off+i*4), buf[i], i);
				break;
			}
		 }
	}
	shell_print(shell,"--cmp  finshed ---\n");

	k_free(buf);
	return 0;

}

#define SPEEDFFSET     	0x20000
#define SPEEDSIZE       0xe0000
static int shell_nor_speed_test(const struct shell *shell,
			      size_t argc, char **argv)
{
	uint32_t  offset,  size;
	char *pend;
	const struct device *nor_dev = device_get_binding(CONFIG_SPI_FLASH_NAME);
	if (!nor_dev) {
		shell_print(shell,"nor dev binding failed!\n");
		return EINVAL;
	}

	if(argc == 3){
		offset =  strtoul(argv[1], &pend, 0);
		size = strtoul(argv[2], &pend, 0);
	}else{
		offset = SPEEDFFSET;
		size = SPEEDSIZE;
	}
	shell_print(shell,"nor test: offset=0x%x, size=0x%x\n", offset, size);
	test_read_speed(shell, nor_dev, offset, size);
	test_write_speed(shell, nor_dev, offset, size);

	return 0;
}


static int shell_flash_read(const struct shell *shell,
					size_t argc, char **argv)
{
	uint32_t  offset, size=1;
	char *pend;
	char *buf;
	int  ret;
	const struct device *f_dev;
	if(argc != 3){
		shell_print(shell,"fread dev_name(sd/spi_flash/spinand) offset\n");
		return -1;
	}
	f_dev = device_get_binding(argv[1]);
	if (!f_dev) {
		shell_print(shell,"flash %s binding failed!\n", argv[1]);
		return EINVAL;
	}
	if(0 == strcmp(argv[1], "spi_flash")) // nor is size else is sector
		size = 512;
	offset =  strtoul(argv[2], &pend, 0);
	buf = k_malloc(READ_BLOCK_ONCE);
	if(buf == NULL){
		shell_print(shell, "malloc fail\n");
		return -1;
	}
	shell_print(shell,"fread 512b: offset=0x%x, buf=0x%x\n", offset, (uint32_t)buf);
	ret = flash_read(f_dev, offset, buf, size);
	if(ret < 0 ) {
		shell_print(shell, "flash read fail\n");
	}else{
		print_buf(shell, buf, 512);
	}
	k_free(buf);
	return 0;

}

#endif

#if defined(CONFIG_CMD_SPINAND)
#define SPINAND_READ_BLOCK_ONCE  2048
static int shell_nand_read(const struct shell *shell,
					size_t argc, char **argv)
{
	uint32_t offset, size;
	char *pend;
	char *buf;
	int  ret;
	const struct device *nand_dev = device_get_binding("spinand");
	if (!nand_dev) {
		shell_print(shell,"nor dev binding failed!\n");
		return EINVAL;
	}
	buf = malloc(SPINAND_READ_BLOCK_ONCE);
	if(buf == NULL){
		shell_print(shell, "malloc fail\n");
		return -1;
	}
	if(argc == 3){
		offset =  strtoul(argv[1], &pend, 0);
		size = strtoul(argv[2], &pend, 0);
	}else{
		shell_print(shell, "offset or size param setting wrong, please check!\n");
		return -1;
	}
	shell_print(shell,"nand read %d: offset=0x%x, buf=0x%x\n", size*512, offset, (uint32_t)buf);
	ret = flash_read(nand_dev, offset, buf, size);
	if(ret < 0 ) {
		shell_print(shell, "flash read fail\n");
	}else{
		print_buf(shell, buf, size*512);
	}
	free(buf);
	return 0;
}

static char wbuf[50] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, \
                        0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x23, 0x37, 0x49, 0x53, \
                        0x68, 0x73, 0x81, 0x95, 0xa3, 0xbd, 0xc8, 0xd7, 0x55, 0xaa};
static int shell_nand_write(const struct shell *shell,
					size_t argc, char **argv)
{
	uint32_t offset, size;
	char *pend;
	int  ret;
	const struct device *nand_dev = device_get_binding("spinand");
	if (!nand_dev) {
		shell_print(shell,"nor dev binding failed!\n");
		return EINVAL;
	}
	if(argc == 3){
		offset = strtoul(argv[1], &pend, 0);
		size = strtoul(argv[2], &pend, 0);
	}else{
		shell_print(shell, "offset or size param setting wrong, please check!\n");
		return -1;
	}
	shell_print(shell,"nand write %d: offset=0x%x \n", size*512, offset*512);
		shell_print(shell, "flash write\n");
	ret = flash_write(nand_dev, offset, wbuf, size);
		shell_print(shell, "flash write finish\n");
	if(ret < 0 ) {
		shell_print(shell, "flash read fail\n");
	}else{
		shell_print(shell, "flash write offset=%d; len=%d succeed!\n", offset, size*512);
	}
	return 0;
}
#endif

#if defined(CONFIG_PM_DIRECT_FORCE_MODE)
#include <pm/pm.h>
static int shell_cmd_sleep(const struct shell *shell,
					size_t argc, char **argv)
{
	static struct pm_state_info  pm_stat;

	if((argc == 2) && !strcmp(argv[1], "s3")) {
		shell_print(shell, "enter deep sleep\n");
		pm_stat.state = PM_STATE_SUSPEND_TO_RAM;
		pm_power_state_force(pm_stat);
		shell_print(shell, "exit deep sleep\n");
	}else{
		shell_print(shell, "enter sleep\n");
		pm_stat.state = PM_STATE_STANDBY;
		pm_power_state_force(pm_stat);
		shell_print(shell, "exit sleep\n");

	}
	return 0;
}

#endif

static int shell_cmd_khead_dump(const struct shell *shell,
					size_t argc, char **argv)
{
	struct k_heap * k_h;
	if(argc == 2){
		k_h = (struct k_heap *) strtoul(argv[1], NULL, 16);
		STRUCT_SECTION_FOREACH(k_heap, h) {
			if(h == k_h){
				shell_print(shell, "dump heap=%p:\n", h);
				sys_heap_dump(&h->heap);
			}else{
				shell_print(shell, "heap=%p != %p\n", h, k_h);
			}
		}
	}else{
		STRUCT_SECTION_FOREACH(k_heap, h) {
			shell_print(shell, "----dump heap=%p:---\n", h);
			sys_heap_dump(&h->heap);
		}
	}

	return 0;
}

#if defined(CONFIG_THREAD_RUNTIME_STATS)

/*
 * cmd: cpuload
 *   start
 *   stop
 */
void cpuload_stat_start(int interval_ms);
void cpuload_stat_stop(void);

static int shell_cmd_cpuload(const struct shell *shell,
					size_t argc, char **argv)
{
	int interval_ms;
	int len = strlen(argv[1]);

	if (!strncmp(argv[1], "start", len)) {
		if (argc > 2)
			interval_ms = strtoul(argv[2], NULL, 0);
		else
			interval_ms = 2000;	/* default interval: 2s */

		shell_print(shell,"Start cpu load statistic, interval %d ms\n",
			interval_ms);

		cpuload_stat_start(interval_ms);

	} else if (!strncmp(argv[1], "stop", len)) {
		shell_print(shell,"Stop cpu load statistic\n");
		cpuload_stat_stop();
	} else {
		shell_print(shell,"usage:\n");
		shell_print(shell,"  cpuload start\n");
		shell_print(shell,"  cpuload stop\n");

		return -EINVAL;
	}

	return 0;
}
#endif	/* CONFIG_CPU_LOAD_STAT */

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
static int shell_cmd_dumpbuf(const struct shell *shell,
					size_t argc, char **argv)
{
	struct fs_file_t zfp;
	uint32_t addr, len;
	int res;

	if (argc < 4) {
		shell_print(shell, "usage:\n");
		shell_print(shell, "  dumpbuf addr len path\n");
		return -EINVAL;
	}

	fs_file_t_init(&zfp);
	res = fs_open(&zfp, argv[3], FS_O_WRITE | FS_O_CREATE);
	if (res < 0) {
		shell_print(shell,"fail to open %s\n", argv[3]);
		return res;
	}

	addr = strtoul(argv[1], NULL, 0);
	len = strtoul(argv[2], NULL, 0);

	fs_write(&zfp, (void *)addr, len);
	fs_close(&zfp);

	shell_print(shell,"done dumping to %s\n", argv[3]);

	return 0;
}
#endif /* CONFIG_FAT_FILESYSTEM_ELM */

#if defined(CONFIG_TRACING_IRQ_PROFILER)

#include <arch/cpu.h>
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#elif defined(CONFIG_CPU_CORTEX_A) || defined(CONFIG_CPU_CORTEX_R)
#include <drivers/interrupt_controller/gic.h>
#endif

void dump_irqstat(void)
{
	struct _isr_table_entry *ite;
	int i;

	printk("IRQ statistics:\n");
	printk("irq  prio    count        max_time (us)        total (us) isr\n");

	for (i = 0; i < IRQ_TABLE_SIZE; i++) {
		ite = &_sw_isr_table[i];

		if (ite->isr != z_irq_spurious) {
		    if(irq_is_enabled(i)){
                printk("%2d(*) %2d   %10d", i, NVIC_GetPriority(i), ite->irq_cnt);
		    }else{
                printk("%2d    %2d   %10d", i, NVIC_GetPriority(i), ite->irq_cnt);
		    }
			printk("    %10d",
			       (u32_t)(k_cyc_to_ns_floor64(ite->max_irq_cycles) / 1000));

			printk("    %10d",
			       ite->irq_total_us);
			printk("\n");
		}
	}
}

static int shell_cmd_irqstat(const struct shell *shell,int argc, char *argv[])
{
	struct _isr_table_entry *ite;
	int i;
	unsigned int key;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

    dump_irqstat();

	if (argc >= 2 && !strncmp(argv[1], "clear", strlen(argv[1]))) {
		key = irq_lock();

		/* clear irq statistics */
		for (i = 0; i < IRQ_TABLE_SIZE; i++) {
			ite = &_sw_isr_table[i];

			if (ite->isr != z_irq_spurious) {
				ite->irq_cnt = 0;
				ite->max_irq_cycles = 0;
				ite->irq_total_us = 0;
			}
		}
		irq_unlock(key);
	}

	return 0;
}
#endif /* CONFIG_IRQ_STAT */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dbg,
#if defined(CONFIG_CMD_MEMORY)
	SHELL_CMD(mdw, NULL, "display memory by word: mdw address [,count]" , shell_cmd_mdw ),
	SHELL_CMD(mdh, NULL, "display memory by half-word: mdh address [,count]" , shell_cmd_mdh),
	SHELL_CMD(mdb, NULL, "display memory by byte: mdb address [,count]" , shell_cmd_mdb),

	SHELL_CMD(mww, NULL, "memory write (fill) by word: mww address value [,count]" , shell_cmd_mww),
	SHELL_CMD(mwh, NULL, "memory write (fill) by half-word: mwh address value [,count]" , shell_cmd_mwh),
	SHELL_CMD(mwb, NULL,"memory write (fill) by byte: mwb address value [,count]" , shell_cmd_mwb),
#endif
#if defined(CONFIG_CMD_SPINOR)
	SHELL_CMD(snort, NULL, "nor test : snort address size", shell_nor_speed_test),
	SHELL_CMD(fread, NULL, "flash read : fread dev[sd/spi_flash/spinand] offset", shell_flash_read),
#endif
#if defined(CONFIG_CMD_SPINAND)
	SHELL_CMD(snandr, NULL, "nand logic read: offset start address; size read len;", shell_nand_read),
	SHELL_CMD(snandw, NULL, "nand logic write: offset start address; size read len;", shell_nand_write),
	//SHELL_CMD(snandlt, NULL, "nand logic test: offset start address; size read len;", shell_nand_logic_speed_test),
#endif
#if defined(CONFIG_PM_DIRECT_FORCE_MODE)
    SHELL_CMD(sleep, NULL, "cpu enter sleep", shell_cmd_sleep),
#endif
	SHELL_CMD(kheap, NULL, "kheap [addr]", shell_cmd_khead_dump),
#if defined(CONFIG_THREAD_RUNTIME_STATS)
	SHELL_CMD(cpuload, NULL, "show cpu load statistic preriodically", shell_cmd_cpuload),
#endif
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
	SHELL_CMD(dumpbuf, NULL, "dump buffer to file system: dumpbuf addr len path", shell_cmd_dumpbuf),
#endif
#if defined(CONFIG_TRACING_IRQ_PROFILER)
	SHELL_CMD(irqstat, NULL, "irqstat (clear)", shell_cmd_irqstat),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(dbg, &sub_dbg, "dbg commands", NULL);
