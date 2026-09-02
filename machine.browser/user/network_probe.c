#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ewoksys/klog.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>

int ewok_network_probe_result = -1;
static int receiver = -1;
static int sender = -1;
static struct sockaddr_in address;
static const char message[] = "ewokos-wasm-loopback";

int ewok_service_init(void) {
    char *interfaces = dev_cmd("/dev/net0", "ip");
    if(interfaces == NULL || strstr(interfaces, "10.0.2.15") == NULL) {
        free(interfaces);
        return -1;
    }
    free(interfaces);
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(23092);
    address.sin_addr.s_un.s_addr = inet_addr("127.0.0.1");

    receiver = socket(AF_INET, SOCK_DGRAM, 0);
    sender = socket(AF_INET, SOCK_DGRAM, 0);
    if(receiver < 0 || sender < 0)
        return -1;
    if(bind(receiver, (struct sockaddr *)&address, sizeof(address)) != 0)
        return -1;
    if(sendto(sender, message, sizeof(message), 0,
            (struct sockaddr *)&address, sizeof(address)) != sizeof(message))
        return -1;
    struct sockaddr_in gateway;
    memset(&gateway, 0, sizeof(gateway));
    gateway.sin_family = AF_INET;
    gateway.sin_port = htons(9);
    gateway.sin_addr.s_un.s_addr = inet_addr("10.0.2.2");
    if(sendto(sender, message, sizeof(message), 0,
            (struct sockaddr *)&gateway, sizeof(gateway)) != sizeof(message))
        return -1;
    return 0;
}

int ewok_service_step(void) {
    char received[sizeof(message)] = {0};
    uint32_t address_size = sizeof(address);

    if(ewok_network_probe_result == 0)
        return 0;
    if((vfs_get_poll_events(receiver) & VFS_EVT_RD) == 0)
        return 0;
    if(recvfrom(receiver, received, sizeof(received), 0,
            (struct sockaddr *)&address, &address_size) != sizeof(message))
        return -1;
    if(memcmp(received, message, sizeof(message)) != 0)
        return -1;

    ewok_network_probe_result = 0;
    klog("network_probe.wasm: UDP loopback passed and Ethernet ARP emitted\n");
    return 0;
}
