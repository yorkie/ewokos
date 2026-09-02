#include <ewoksys/syscall.h>
#include <stdint.h>

extern int32_t ewok_syscall(int32_t code, uint32_t arg0, uint32_t arg1,
        uint32_t arg2);

ewokos_addr_t syscall3_raw(int code, ewokos_addr_t arg0,
        ewokos_addr_t arg1, ewokos_addr_t arg2) {
    return (ewokos_addr_t)ewok_syscall(code, arg0, arg1, arg2);
}
