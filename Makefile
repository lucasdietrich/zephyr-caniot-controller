# GENERATOR=Unix Makefiles
# GENERATOR=Ninja
GENERATOR=Ninja

PYOCD = pyocd

ifeq ($(GENERATOR),Unix Makefiles)
GEN_CMD=make
GEN_OPT=--no-print-directory
else
GEN_CMD=ninja
GEN_OPT=
endif

# Get ap^plication VERSION
VERSION := $(shell cat VERSION)

all: build make

tmp:
	mkdir -p tmp

# Activate python env variable before building: "source ../.venv/bin/activate "
# add option "--cmake-only" to not build immediately
build:
	west build --board=nucleo_f429zi -- -G"$(GENERATOR)"

build_nucleo_f429zi_ramfatfs:
	west build --board=nucleo_f429zi -- -DOVERLAY_CONFIG="overlays/f429zi_ram_fatfs.conf" -G"$(GENERATOR)"

build_nucleo_f429zi_ramfatfs_shell:
	west build --board=nucleo_f429zi -- -DOVERLAY_CONFIG="overlays/f429zi_ram_fatfs.conf overlays/f429zi_shell.conf" -G"$(GENERATOR)"

build_nucleo_f429zi_shell:
	west build --board=nucleo_f429zi -- -DOVERLAY_CONFIG="overlays/f429zi_shell.conf" -G"$(GENERATOR)"

build_qemu:
	west build --board=qemu_x86 -- -G"$(GENERATOR)"

flash:
	west flash

make: build
	${GEN_CMD} -C build $(GEN_OPT)

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
	python3 ./modules/embedc-utils/scripts/genroutes.py \
		src/http_server/routes.txt \
		--output=src/http_server/routes_g.c \
		--descr-whole \
		--def-begin="=== ROUTES DEFINITION BEGIN  === */" \
		--def-end="/* === ROUTES DEFINITION END === */"