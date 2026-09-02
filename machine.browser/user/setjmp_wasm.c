#include <setjmp.h>

/* WebAssembly has no native stack-register image matching EwokOS's assembly
 * implementations.  FreeType uses this pair only as an error escape while
 * parsing/rasterizing; valid bundled fonts stay on the zero-return path. */
int setjmp(jmp_buf env) {
    (void)env;
    return 0;
}

void longjmp(jmp_buf env, int value) {
    (void)env;
    (void)value;
    __builtin_trap();
}
