# Copyright (c) 2018, Sergiy Yevtushenko
# Copyright (c) 2018-2019, Niklas Hauser
#
# This file is part of the modm project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

BUILD_DIR = build

CMAKE_GENERATOR = Unix Makefiles
CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON -DCMAKE_RULE_MESSAGES:BOOL=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF

.PHONY: cmake build clean cleanall program program-bmp debug debug-bmp debug-coredump log-itm

.DEFAULT_GOAL := all

### Targets
all: cmake build

cmake:
	@cmake -E make_directory $(BUILD_DIR)/cmake-build-debug
	@cmake -E make_directory $(BUILD_DIR)/cmake-build-release
	@cd $(BUILD_DIR)/cmake-build-debug && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug -G "$(CMAKE_GENERATOR)" ../..
	@cd $(BUILD_DIR)/cmake-build-release && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release -G "$(CMAKE_GENERATOR)" ../..

clean:
	@cmake --build $(BUILD_DIR)/cmake-build-release --target clean
	@cmake --build $(BUILD_DIR)/cmake-build-debug --target clean

cleanall:
	@rm -rf $(BUILD_DIR)/cmake-build-release
	@rm -rf $(BUILD_DIR)/cmake-build-debug

profile?=release
build:
	@cmake --build $(BUILD_DIR)/cmake-build-$(profile)

port?=auto
ELF_FILE=$(BUILD_DIR)/cmake-build-$(profile)/purpledrop.elf
MEMORIES = "[{'name': 'flash', 'access': 'rx', 'start': 134217728, 'size': 1572864}, {'name': 'ccm', 'access': 'rw', 'start': 268435456, 'size': 65536}, {'name': 'sram1', 'access': 'rwx', 'start': 536870912, 'size': 327680}, {'name': 'backup', 'access': 'rwx', 'start': 1073889280, 'size': 4096}]"
size: build
	@python3 modm/modm_tools/size.py $(ELF_FILE) $(MEMORIES)

program: build
	@python3 modm/modm_tools/openocd.py -f modm/openocd.cfg $(ELF_FILE)

program-bmp: build
	@python3 modm/modm_tools/bmp.py -p $(port) $(ELF_FILE)

gdb: build
	gdb-multiarch -q -x openocd.gdb

docs:
	(cd modm/docs && doxygen doxyfile.cfg)
	open modm/docs/html/index.html

ui?=tui
debug: build
	@python3 modm/modm_tools/gdb.py -x modm/gdbinit -x modm/openocd_gdbinit \
			$(ELF_FILE) -ui=$(ui) \
			openocd -f modm/openocd.cfg

debug-bmp: build
	@python3 modm/modm_tools/bmp.py -x modm/gdbinit \
			$(ELF_FILE) -ui=$(ui) \
			bmp -p $(port)

debug-coredump: build
	@python3 modm/modm_tools/gdb.py -x modm/gdbinit \
			$(ELF_FILE) -ui=$(ui) \
			crashdebug --binary-path modm/ext/crashcatcher/bins

fcpu?=0
log-itm:
	@python3 modm/modm_tools/log.py itm openocd -f modm/openocd.cfg -fcpu $(fcpu)
