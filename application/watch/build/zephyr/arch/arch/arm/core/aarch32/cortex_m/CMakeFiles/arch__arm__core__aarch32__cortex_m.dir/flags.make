# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# compile ASM with /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
# compile C with /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
ASM_FLAGS =   -Og -imacros /home/wt/zephyr-kit/project_wkpace/build/zephyr/include/generated/autoconf.h -ffreestanding -fno-common -g -fdiagnostics-color=always -mcpu=cortex-m4 -mthumb -mabi=aapcs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -xassembler-with-cpp -imacros /home/wt/zephyr-kit/zephyr_pkg/zephyr/include/toolchain/zephyr_stdint.h -D_ASMLANGUAGE -Wno-unused-but-set-variable -fno-asynchronous-unwind-tables -fno-pie -fno-pic -fno-strict-overflow -fno-reorder-functions -fno-defer-pop -fmacro-prefix-map=/home/wt/zephyr-kit/project_wkpace=CMAKE_SOURCE_DIR -fmacro-prefix-map=/home/wt/zephyr-kit/zephyr_pkg/zephyr=ZEPHYR_BASE -fmacro-prefix-map=/home/wt/zephyr-kit/zephyr_pkg=WEST_TOPDIR -ffunction-sections -fdata-sections -specs=nano.specs

ASM_DEFINES = -DAM_HAL_DISABLE_API_VALIDATION -DAM_PACKAGE_BGA -DAM_PART_APOLLO3P -DBUILD_VERSION=zephyr-v2.6.0 -DCORE_CM4 -DGX_DISABLE_DEPRECATED_STRING_API -DGX_DISABLE_THREADX_BINDING -DKERNEL -D_FORTIFY_SOURCE=2 -D__LINUX_ERRNO_EXTENSIONS__ -D__PROGRAM_START -D__ZEPHYR_SUPERVISOR__ -D__ZEPHYR__=1

ASM_INCLUDES = -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/kernel/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/arch/arm/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/include -I/home/wt/zephyr-kit/project_wkpace/build/zephyr/include/generated -I/home/wt/zephyr-kit/project_wkpace/../bsps/soc/arm/ambiq_apollo3p -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/lib/libc/newlib/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/lib/util/fnmatch/. -I/home/wt/zephyr-kit/bsps/soc/arm/CMSIS/CMSIS/Core/Include -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/AmbiqMicro/Include -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/mcu/apollo3p -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/mcu/apollo3p/regs -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/mcu/apollo3p/hal -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/utils -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/settings/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth -I/home/wt/zephyr-kit/bsps/drivers/zephyr/. -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/drivers -I/home/wt/zephyr-kit/bsps/drivers/zephyr/kscan/cyttsp5_mcu -I/home/wt/zephyr-kit/bsps/drivers_ext/. -I/home/wt/zephyr-kit/bsps/drivers_ext/../include -I/home/wt/zephyr-kit/components/. -I/home/wt/zephyr-kit/components/gui/gui/common/inc -I/home/wt/zephyr-kit/components/gui/. -I/home/wt/zephyr-kit/components/gui/guix_widget/. -I/home/wt/zephyr-kit/zephyr_pkg/tinycrypt/lib/include 

C_FLAGS =   -Og -imacros /home/wt/zephyr-kit/project_wkpace/build/zephyr/include/generated/autoconf.h -ffreestanding -fno-common -g -fdiagnostics-color=always -mcpu=cortex-m4 -mthumb -mabi=aapcs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -imacros /home/wt/zephyr-kit/zephyr_pkg/zephyr/include/toolchain/zephyr_stdint.h -Wall -Wformat -Wformat-security -Wno-format-zero-length -Wno-main -Wno-pointer-sign -Wpointer-arith -Wexpansion-to-defined -Wno-address-of-packed-member -Wno-unused-but-set-variable -Werror=implicit-int -fno-asynchronous-unwind-tables -fno-pie -fno-pic -fno-strict-overflow -fno-reorder-functions -fno-defer-pop -fmacro-prefix-map=/home/wt/zephyr-kit/project_wkpace=CMAKE_SOURCE_DIR -fmacro-prefix-map=/home/wt/zephyr-kit/zephyr_pkg/zephyr=ZEPHYR_BASE -fmacro-prefix-map=/home/wt/zephyr-kit/zephyr_pkg=WEST_TOPDIR -ffunction-sections -fdata-sections -specs=nano.specs -std=c99

C_DEFINES = -DAM_HAL_DISABLE_API_VALIDATION -DAM_PACKAGE_BGA -DAM_PART_APOLLO3P -DBUILD_VERSION=zephyr-v2.6.0 -DCORE_CM4 -DGX_DISABLE_DEPRECATED_STRING_API -DGX_DISABLE_THREADX_BINDING -DKERNEL -D_FORTIFY_SOURCE=2 -D__LINUX_ERRNO_EXTENSIONS__ -D__PROGRAM_START -D__ZEPHYR_SUPERVISOR__ -D__ZEPHYR__=1

C_INCLUDES = -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/kernel/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/arch/arm/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/include -I/home/wt/zephyr-kit/project_wkpace/build/zephyr/include/generated -I/home/wt/zephyr-kit/project_wkpace/../bsps/soc/arm/ambiq_apollo3p -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/lib/libc/newlib/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/lib/util/fnmatch/. -I/home/wt/zephyr-kit/bsps/soc/arm/CMSIS/CMSIS/Core/Include -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/AmbiqMicro/Include -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/mcu/apollo3p -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/mcu/apollo3p/regs -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/mcu/apollo3p/hal -I/home/wt/zephyr-kit/bsps/soc/arm/ambiq_apollo3p/apollo_mcu_lib/utils -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/settings/include -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth -I/home/wt/zephyr-kit/bsps/drivers/zephyr/. -I/home/wt/zephyr-kit/zephyr_pkg/zephyr/drivers -I/home/wt/zephyr-kit/bsps/drivers/zephyr/kscan/cyttsp5_mcu -I/home/wt/zephyr-kit/bsps/drivers_ext/. -I/home/wt/zephyr-kit/bsps/drivers_ext/../include -I/home/wt/zephyr-kit/components/. -I/home/wt/zephyr-kit/components/gui/gui/common/inc -I/home/wt/zephyr-kit/components/gui/. -I/home/wt/zephyr-kit/components/gui/guix_widget/. -I/home/wt/zephyr-kit/zephyr_pkg/tinycrypt/lib/include 

