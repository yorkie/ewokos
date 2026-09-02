#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/klog.h>
#include <ewoksys/vfs.h>

static int32_t probe_result = -1;

int32_t ewok_rootfs_probe_result(void) {
    return probe_result;
}

int ewok_service_init(void) {
    char buffer[96];
    static const char ramfs_data[] = "native wasm ramfs\n";
    int fd;
    int count;

    probe_result = -20;
    fd = open("/etc/init.rd", O_RDONLY);
    if(fd < 0) {
        probe_result = -200 - errno;
        return -1;
    }
    probe_result = -30;
    count = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if(count <= 0)
        return -1;
    buffer[count] = 0;
    klog("rootfs.wasm: read /etc/init.rd through vfsd+sdfsd (%d bytes)\n",
        count);

    probe_result = -40;
    fd = open("/dev/null", O_RDWR);
    if(fd < 0 || write(fd, ramfs_data, sizeof(ramfs_data) - 1) !=
            (int)(sizeof(ramfs_data) - 1))
        return -1;
    close(fd);

    probe_result = -50;
    fd = open("/tmp/wasm-io", O_CREAT | O_RDWR);
    if(fd < 0 || write(fd, ramfs_data, sizeof(ramfs_data) - 1) !=
            (int)(sizeof(ramfs_data) - 1))
        return -1;
    lseek(fd, 0, SEEK_SET);
    memset(buffer, 0, sizeof(buffer));
    if(read(fd, buffer, sizeof(ramfs_data) - 1) !=
            (int)(sizeof(ramfs_data) - 1) ||
            memcmp(buffer, ramfs_data, sizeof(ramfs_data) - 1) != 0) {
        close(fd);
        return -1;
    }
    close(fd);

    probe_result = -60;
    int pipefd[2];
    if(pipe(pipefd) != 0 ||
            write(pipefd[1], ramfs_data, sizeof(ramfs_data) - 1) !=
                (int)(sizeof(ramfs_data) - 1))
        return -1;
    memset(buffer, 0, sizeof(buffer));
    if(read(pipefd[0], buffer, sizeof(ramfs_data) - 1) !=
            (int)(sizeof(ramfs_data) - 1) ||
            memcmp(buffer, ramfs_data, sizeof(ramfs_data) - 1) != 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    close(pipefd[0]);
    close(pipefd[1]);
    probe_result = count;
    klog("io.wasm: null, ramfs and pipe read/write passed (%d)\n", count);
    return 0;
}

int ewok_service_step(void) {
    return 0;
}
