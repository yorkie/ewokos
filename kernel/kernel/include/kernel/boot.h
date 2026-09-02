#ifndef __KERNEL_BOOT_H__
#define __KERNEL_BOOT_H__

#ifdef __wasm__
extern char __global_base;
extern char __heap_base;
#define _kernel_start (&__global_base)
#define _kernel_end (&__heap_base)
#else
extern char _kernel_start[];
extern char _kernel_end[];
#endif
extern char _kernel_sp[];

#ifndef __wasm__
extern char _bss_start[];
extern char _bss_end[];
#endif

#endif
