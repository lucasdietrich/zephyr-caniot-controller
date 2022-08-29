read_bank0:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -f read_bank0.cfg
	hexdump -C bank0.bin > bank0.hex

write_cert:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -f write_cert.cfg

erase_certificate_sector:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -f erase_certificate_sector.cfg

mass_erase:
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init" -c "halt" -c "wait_halt" -c "stm32f4x mass_erase 0" -c "sleep 200" -c "reset run" -c "shutdown"