#include <dev/uart.h>

extern void console_write(const char*, int);

int32_t uart_dev_init(uint32_t baud) {
	(void)baud;
	return 0;
}

int32_t uart_write(const void* data, uint32_t size) {
	console_write((const char*)data, (int)size);
	return (int32_t)size;
}
