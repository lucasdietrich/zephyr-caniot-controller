# GENERATOR=Unix Makefiles
# GENERATOR=Ninja
GENERATOR?=Ninja
SERIAL_PORT?=/dev/ttyACM0
BAUDRATE?=115200
PYOCD?= pyocd
BUILD_TYPE?=debug

ifeq ($(GENERATOR),Unix Makefiles)
GEN_CMD=make
GEN_OPT=--no-print-directory
else
GEN_CMD=ninja
GEN_OPT=
endif

# Get ap^plication VERSION
VERSION := $(shell cat VERSION)

.PHONY: release debug

all: debug

tmp:
	mkdir -p tmp

# Activate python env variable before building: "source ../.venv/bin/activate "
# add option "--cmake-only" to not build immediately
release:
	west build --board=nucleo_f429zi -- \
	-DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay" \
	-DOVERLAY_CONFIG="overlays/nucleo_f429zi_mcuboot.conf" \
		-G"$(GENERATOR)"

debug:
	west build -b nucleo_f429zi -- -G"$(GENERATOR)"

ninja:
	ninja -C build

	
flash:
	west flash --runner=openocd

# FIXME: compile but MCUBOOT doesn't recognize the image
build_debug:
	west build --board=nucleo_f429zi -- \
		-DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay" \
		-DOVERLAY_CONFIG="overlays/nucleo_f429zi_shell.conf" \
		-G"$(GENERATOR)"

build_nucleo_f429zi_loggingfs:
	west build --board=nucleo_f429zi -- \
		-DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay" \
		-DOVERLAY_CONFIG="overlays/nucleo_f429zi_logging_fs.conf" \
		-G"$(GENERATOR)"

build_nucleo_f429zi_ramfatfs:
	west build --board=nucleo_f429zi -- \
		-DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay overlays/nucleo_f429zi_ram_fatfs.overlay" \
		-DOVERLAY_CONFIG="overlays/nucleo_f429zi_ram_fatfs.conf" \
		-G"$(GENERATOR)"

build_nucleo_f429zi_ramfatfs_shell:
	west build --board=nucleo_f429zi -- \
	-DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay overlays/nucleo_f429zi_ram_fatfs.overlay" \
	-DOVERLAY_CONFIG="overlays/nucleo_f429zi_ram_fatfs.conf overlays/nucleo_f429zi_shell.conf" \
	-G"$(GENERATOR)"

build_nucleo_f429zi_shell:
	west build --board=nucleo_f429zi -- \
	-DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay" \
	-DOVERLAY_CONFIG="overlays/nucleo_f429zi_shell.conf" -G"$(GENERATOR)"

build_qemu_arm_slip:
	west build --board=mps2_an385 -- \
	-DOVERLAY_CONFIG="overlays/mps2_an385_slip.conf" \
	-DDTC_OVERLAY_FILE="boards/mps2_an385.overlay overlays/mps2_an385_slip.overlay" \
	-G"$(GENERATOR)"

# Doesn't work yet (TODO)
build_qemu_arm_lan9220:
	west build --board=mps2_an385 -- \
	-DOVERLAY_CONFIG="overlays/mps2_an385_lan9220.conf" \
	-DDTC_OVERLAY_FILE="boards/mps2_an385.overlay overlays/mps2_an385_lan9220.overlay" \
	-G"$(GENERATOR)"

build_qemu_x86:
	west build --board=qemu_x86 -- -DDTC_OVERLAY_FILE="boards/qemu_x86.overlay" -G"$(GENERATOR)"

# Ctrl-T Q to quit
monitor:
	python3 -m serial.tools.miniterm --filter=direct ${SERIAL_PORT} ${BAUDRATE}

monitor_2:
	python3 -m serial.tools.miniterm ${SERIAL_PORT} ${BAUDRATE}

reports: tmp
	${GEN_CMD} -C build ram_report $(GEN_OPT) > docs/ram_report.txt
	${GEN_CMD} -C build rom_report $(GEN_OPT) > docs/rom_report.txt

debugserver:
	ninja debugserver -C build

run:
	west build -t run

clean:
	rm -rf build

creds_hardcoded:
	python3 ./scripts/creds/hardcoded_creds_xxd.py

sign:
	west sign -t imgtool -- \
		--key creds/mcuboot/root-ec-p256.pem \
		--pad \
		--version $(VERSION)

sign_complete:
	west sign -t imgtool -- \
		--key creds/mcuboot/root-ec-p256.pem \
		--header-size 0x200 \
		--align 4 \
		--version 1.2 \
		--slot-size 0xc0000 \
		--pad \
		--version $(VERSION)

signed_info:
	hexdump build/zephyr/zephyr.signed.bin -C -n 0x200

# Caution: this will erase the whole flash
flash_bootloader:
	$(PYOCD) flash -e chip -a 0x08000000 bins/bootloader_debug.bin

flash_slot0:
	$(PYOCD) flash -a 0x08020000 build/zephyr/zephyr.signed.bin

flash_slot1:
	$(PYOCD) flash -a 0x08120000 build/zephyr/zephyr.signed.bin

flash_upgrade: flash_slot1

dis:
	./scripts/dis.sh

http_routes_generate:
	python3 ./modules/embedc-url/scripts/genroutes.py \
		src/http_server/routes.txt \
		--output=src/http_server/routes_g.c \
		--descr-whole \
		--def-begin="=== ROUTES DEFINITION BEGIN  === */" \
		--def-end="/* === ROUTES DEFINITION END === */"
	ls src/http_server/routes_g.c | xargs clang-format -i

zephyr_conf_synthesis:
	python3 ./scripts/zephyr_conf_parser.py

format:
	find src -iname *.h -o -iname *.c -o -iname *.cpp | xargs clang-format -i

menuconfig:
	west build -t menuconfig