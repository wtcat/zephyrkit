#include <autoconf.h>
#include <devicetree.h>

MEMORY {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(psram0), okay)
    PSRAM    (rw) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(psram0)), LENGTH = DT_REG_SIZE(DT_NODELABEL(psram0))
#endif
}

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
