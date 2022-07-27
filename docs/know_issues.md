## Know issues

## Missing `sig_atomic_t` for `<lua/lstate.h>`

```c
/* TODO where to find sig_atomic_t ??
 *
 * "lua/lstate.h" needs it but the header <signal.h> included for the application
 * seems to be different from the one included for the lua module/library.
 * 
 * for x86 for example, this header includes it:
 *  /home/lucas/zephyr-sdk-0.14.2/x86_64-zephyr-elf/x86_64-zephyr-elf/include/signal.h
 *  /home/lucas/zephyr-sdk-0.14.2/x86_64-zephyr-elf/x86_64-zephyr-elf/sys-include/signal.h
 * for arm:
 *  /home/lucas/zephyr-sdk-0.14.2/arm-zephyr-eabi/arm-zephyr-eabi/include/signal.h
 *  /home/lucas/zephyr-sdk-0.14.2/arm-zephyr-eabi/arm-zephyr-eabi/sys-include/signal.h
 * 
 * So a need to define it here ...
 */
// typedef int	sig_atomic_t;		/* Atomic entity type (ANSI) */
```