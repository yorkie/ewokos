ARCH = wasm
EWOKOS_ROOT_DIR = ../..

include make.bsp

OS_IMG = kernel.wasm
OS_ELF = kernel.elf

include $(EWOKOS_ROOT_DIR)/kernel/platform/wasm/make.rule

OBJS += $(BSP_OBJS)

LDFLAGS += -matomics -mbulk-memory -Wl,--import-memory -Wl,--shared-memory -nostdlib

bsp/uart.o: bsp/uart.c
	$(CC) $(CFLAGS) -c -o $@ $<

bsp/start.o: bsp/start.c
	$(CC) $(CFLAGS) -c -o $@ $<

bsp/sd.o: bsp/sd.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OS_IMG)

$(OS_IMG): $(BSP_OBJS)
