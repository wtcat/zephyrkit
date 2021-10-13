#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "wsf_timer.h"

#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "hci_core.h"
#include "hci_drv.h"
#include "hci_drv_apollo.h"
#include "hci_drv_apollo3.h"
#include "hci_apollo_config.h"

#include "wsf_msg.h"

//#include "ancc_api.h"
//#include "app_ui.h"

//run demo
#include "amdtp_api.h"

#include <init.h>
#include <kernel.h>
#include <soc.h>

#ifdef CONFIG_BLE_USERSPACE
#define CONFIG_BLE_THREAD_FLAGS K_USER
#else
#define CONFIG_BLE_THREAD_FLAGS 0
#endif /* CONFIG_BLE_USERSPACE */

/*
 * Important note: the size of wsf_memory_area should includes both overhead of internal
 * buffer management structure, wsfBufPool_t (up to 16 bytes for each pool), and pool
 * description (e.g. wfs_bufinfo below).
 *
 * Memory for the buffer pool
 * extra AMOTA_PACKET_SIZE bytes for OTA handling
 */
#define WSF_BUF_POOLS  4
#define WSF_MEM_SIZE   (WSF_BUF_POOLS*16 + 16*8 + 32*4 + 64*6 + 280*8)

static uint32_t wsf_memory_area[WSF_MEM_SIZE / sizeof(uint32_t)];
static wsfBufPoolDesc_t wfs_bufinfo[WSF_BUF_POOLS] = {
    {16,  8}, {32,  4}, {64,  6}, {280, 8} 
};

static K_THREAD_STACK_DEFINE(ble_stack, CONFIG_BLE_THREAD_STACK);
static struct k_thread ble_thread;

void __weak wsf_main(void)
{
    printk("BLE warning***: Please redefine \"int wsf_main(void)\" \n"
        "to register event and initialize application\n");
    printk("eg: int wsf_main(void)\n"
           "    {\n"
           "        wsfHandlerId_t hid;\n"
           "        hid = WsfOsSetNextHandler(ble_app_Handler);\n"
           "        ble_app_HandlerInit(hid);\n"
           "        ...\n"
           "        ble_app_start();\n"
           "    }\n");
}

static void ble_isr(const void *parameter)
{
    ARG_UNUSED(parameter);
    
    HciDrvIntService();
    WsfTaskSetReady(0, 0);
}

static int ble_stack_init(void)
{   
    wsfHandlerId_t hid;
    size_t len; 
    
    /* Set up timers for the WSF scheduler. */
    WsfOsInit();
    WsfTimerInit();

    /* Initialize a buffer pool for WSF dynamic memory needs. */
    len = WsfBufInit(sizeof(wsf_memory_area), 
                     (uint8_t *)wsf_memory_area,
                     WSF_BUF_POOLS, 
                     wfs_bufinfo);
    if (len > sizeof(wsf_memory_area)) {
        printk("Memory pool is too small by %d\r\n", 
            len - sizeof(wsf_memory_area));
        return -ERANGE;
    }

    /* Initialize the WSF security service. */
    SecInit();
    SecAesInit();
    SecCmacInit();
    SecEccInit();

    /* Set up callback functions for the various layers of 
     * the ExactLE stack. 
     */
    hid = WsfOsSetNextHandler(HciHandler);
    HciHandlerInit(hid);

    hid = WsfOsSetNextHandler(DmHandler);
    DmDevVsInit(0);
    DmAdvInit();
    DmConnInit();
    DmConnSlaveInit();
    DmSecInit();
    DmSecLescInit();
    DmPrivInit();
    DmHandlerInit(hid);

    hid = WsfOsSetNextHandler(L2cSlaveHandler);
    L2cSlaveHandlerInit(hid);
    L2cInit();
    L2cSlaveInit();

    hid = WsfOsSetNextHandler(AttHandler);
    AttHandlerInit(hid);
    AttsInit();
    AttsIndInit();
    AttcInit();

    hid = WsfOsSetNextHandler(SmpHandler);
    SmpHandlerInit(hid);
    SmprInit();
    SmprScInit();
    HciSetMaxRxAclLen(251);

    hid = WsfOsSetNextHandler(AppHandler);
    AppHandlerInit(hid);

    //hid = WsfOsSetNextHandler(ble_app_Handler);
    //ble_app_HandlerInit(hid);
    hid = WsfOsSetNextHandler(AmdtpHandler);
    AmdtpHandlerInit(hid);

    hid = WsfOsSetNextHandler(HciDrvHandler);
    HciDrvHandlerInit(hid);
    return 0;
}

static void ble_radio_thread(void *arg)
{
#if WSF_TRACE_ENABLED == TRUE
    printk("Starting wicentric trace:\n\n");
#endif

    /* Initialize BLE stack */
    if (ble_stack_init()) {
        printk("BLE thread exited.");
        k_thread_abort((k_tid_t)arg);
    }
    
    /* Application initialize */
    //wsf_main();
    AmdtpStart();

    for ( ; ; ) {
        /*
         * Calculate the elapsed time from our free-running timer, and update
         * the software timers in the WSF scheduler.
         */
        wsfOsDispatcher();
    }
}

static int ble_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    k_tid_t thread;

    irq_connect_dynamic(BLE_IRQn, 0, ble_isr, NULL, 0);
    irq_enable(BLE_IRQn);

    /* Boot the radio. */
    HciDrvRadioBoot(1);

    /* Create BLE thread */
    thread = k_thread_create(&ble_thread, ble_stack, 
        K_THREAD_STACK_SIZEOF(ble_stack), 
        (k_thread_entry_t)ble_radio_thread,
        &ble_thread, NULL, NULL,
        CONFIG_BLE_THREAD_PRIO, 
        CONFIG_BLE_THREAD_FLAGS, 
        K_FOREVER);
    k_thread_name_set(thread, "BLE-THREAD");
    k_thread_start(thread);
    return 0;
}

SYS_INIT(ble_init, APPLICATION, 
    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT+10);

