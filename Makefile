# GENERATOR=Unix Makefiles
# GENERATOR=Ninja
GENERATOR=Ninja

ifeq ($(GENERATOR),Unix Makefiles)
GEN_CMD=make
GEN_OPT=--no-print-directory
else
GEN_CMD=ninja
GEN_OPT=
endif

all: build make

tmp:
	mkdir -p tmp

# Activate python env variable before building: "source ../.venv/bin/activate "
# add option "--cmake-only" to not build immediately
build:
	west build --board=nucleo_f429zi -- -G"$(GENERATOR)"

build_nucleo_f429zi_ramfatfs:
	west build --board=nucleo_f429zi -- -DOVERLAY_CONFIG="boards/f429zi_ram_fatfs.conf" -G"$(GENERATOR)"

build_nucleo_f429zi_ramfatfs_shell:
	west build --board=nucleo_f429zi -- -DOVERLAY_CONFIG="boards/f429zi_ram_fatfs.conf boards/f429zi_shell.conf" -G"$(GENERATOR)"

build_nucleo_f429zi_shell:
	west build --board=nucleo_f429zi -- -DOVERLAY_CONFIG="boards/f429zi_shell.conf" -G"$(GENERATOR)"

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