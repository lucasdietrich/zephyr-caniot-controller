# Embedding binaries in application

## PlatformIO specificities 

- https://community.platformio.org/t/board-build-embed-txtfiles-does-not-create-symbol-names/13227/5
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#embedding-binary-data

## Zephyr : 
Tried this https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/net/sockets/echo_server/CMakeLists.txt#L31-L43 :
```
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(inc_file
	ca.der
	server.der
	server_privkey.der
	echo-apps-cert.der
	echo-apps-key.der
    )
  generate_inc_file_for_target(
    app
    src/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()

```


Had warning : 
```
Warning! Detected a custom CMake command for embedding files. Please use 'board_build.embed_files' option in 'platformio.ini' to include files!
```