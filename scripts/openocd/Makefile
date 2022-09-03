read_bank0:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -f read_bank0.cfg
	hexdump -C bank0.bin > bank0.hex

read_credentials:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -f read_credentials_sector.cfg
	hexdump -C tmp_openocd_read.bin > tmp_openocd_read.hex

read_credentials1:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c \
	"init; reset halt; flash read_bank 0 tmp_openocd_read.bin 0x1e0000 0x20000; reset halt; shutdown;"
	hexdump -C tmp_openocd_read.bin > tmp_openocd_read.hex

read_credentials2:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c \
	"init" -c "reset halt" -c "flash read_bank 0 tmp_openocd_read.bin 0x1e0000 0x20000" -c "reset halt" -c "shutdown"
	hexdump -C tmp_openocd_read.bin > tmp_openocd_read.hex

write_cert:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -f write_cert.cfg

erase_credentials_sector:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -f erase_credentials_sector.cfg

mass_erase:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init" -c "halt" -c "wait_halt" -c "stm32f4x mass_erase 0" -c "sleep 200" -c "reset run" -c "shutdown"