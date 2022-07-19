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
	west build --board=nucleo_f429zi -- -DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay" -DCONF_FILE="prj.conf" -DOVERLAY_CONFIG="nucleo_f429zi.conf" -G"$(GENERATOR)"

build_qemu:
	west build --board=qemu_x86 -- -DCONF_FILE="prj.conf" -G"$(GENERATOR)"

flash:
	west flash

make: build
	${GEN_CMD} -C build $(GEN_OPT)

reports: tmp
	${GEN_CMD} -C build ram_report $(GEN_OPT) > tmp/ram_report.txt
	${GEN_CMD} -C build rom_report $(GEN_OPT) > tmp/rom_report.txt

debugserver:
	ninja debugserver -C build

run:
	west build -t run

clean:
	rm -rf build