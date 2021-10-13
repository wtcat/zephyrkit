BOARD ?= iwatchv0

BUILD_DIRECTORY := $(CURDIR)/build/$(BOARD)
BUILD_DIR_WATCH_NO_BOOT := $(BUILD_DIRECTORY)/watch_no_boot

SOURCE_DIRECTORY := $(CURDIR)

watch_no_boot: check
	(mkdir -p $(BUILD_DIR_WATCH_NO_BOOT) && \
		cd $(BUILD_DIR_WATCH_NO_BOOT) && \
		cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
			-DBOARD=$(BOARD) \
			$(SOURCE_DIRECTORY)/../project_wkpace/ && \
		make -j4)

config: check
		(mkdir -p $(BUILD_DIR_WATCH_NO_BOOT) && \
		cd $(BUILD_DIR_WATCH_NO_BOOT) && \
		cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
			-DBOARD=$(BOARD) \
			$(SOURCE_DIRECTORY)/../project_wkpace/)

submodule_update: check
	rm -rf ../zephyr_pkg/modules
	west config manifest.path  ../project_wkpace  && \
	west update

menuconfig:check
	make -C $(BUILD_DIR_WATCH_NO_BOOT) menuconfig

debug:check
	make -C $(BUILD_DIR_WATCH_NO_BOOT) gdbserver & \
	make -C $(BUILD_DIR_WATCH_NO_BOOT) gdb

check:
	@if [ -z "$$ZEPHYR_BASE" ]; then echo "Zephyr environment not set up"; false; fi
	@if [ -z "$(BOARD)" ]; then echo "You must specify BOARD=<board>"; false; fi
