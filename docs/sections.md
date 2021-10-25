# Sections (& partitions)

- [Code And Data Relocation](https://docs.zephyrproject.org/latest/guides/code-relocation.html)
- [Iterable Sections](https://docs.zephyrproject.org/latest/reference/iterable_sections/index.html)
- Putting code in a specific sections :
  - [`include\linker\section_tags.h`](https://github.com/nrfconnect/sdk-zephyr/blob/master/include/linker/section_tags.h)

Examples :

- MBEDTLS heap : 

```c
__ccm_bss_section char heap[0x10000];
```

- No init :

```c
__noinit char tmp[0x1000];
```