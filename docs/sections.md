# Sections (& partitions)

Putting code in a specific section :

- Check for file [`include\linker\section_tags.h`](https://github.com/nrfconnect/sdk-zephyr/blob/master/include/linker/section_tags.h)

Examples :

- MBEDTLS heap : 

```c
__ccm_bss_section char heap[0x10000];
```

- No init :

```c
__noinit char tmp[0x1000];
```