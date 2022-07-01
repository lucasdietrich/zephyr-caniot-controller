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
build:
	west build --board=nucleo_f429zi --cmake-only -- -DDTC_OVERLAY_FILE="nucleo_f429zi.overlay ipc_to_custom_acn52840.overlay" -G"$(GENERATOR)"

flash:
	west flash

make: build
	${GEN_CMD} -C build $(GEN_OPT)

reports: tmp
	${GEN_CMD} -C build ram_report $(GEN_OPT) > tmp/ram_report.txt
	${GEN_CMD} -C build rom_report $(GEN_OPT) > tmp/rom_report.txt

clean:
	rm -rf build