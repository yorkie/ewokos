#ifndef EWOKOS_CONFIG_H
#define EWOKOS_CONFIG_H

// WebAssembly configuration for EwokOS
#define PAGE_SIZE     4096

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#endif

// Disable EwokOS-specific features
#define WITH_KERNEL_VFS 0
#define WITH_PROC_INFO 0

// Enable standard features
#define KMALLOC_SIZE (32*1024*1024)

#endif // EWOKOS_CONFIG_H