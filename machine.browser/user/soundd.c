#include <stdint.h>
#include <string.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>

#define CTRL_PCM_DEV_HW 0xF0
#define CTRL_PCM_DEV_HW_FREE 0xF1
#define CTRL_PCM_DEV_PRPARE 0xF2
#define CTRL_PCM_BUF_AVAIL 0xF3
#define CTRL_SND_SET_VOLUME 0xF8
#define CTRL_SND_GET_VOLUME 0xF9

struct pcm_config {
    int bit_depth;
    int rate;
    int channels;
    int period_size;
    int period_count;
    int start_threshold;
    int stop_threshold;
};

extern int32_t wasm_host_audio_write(const void *data, uint32_t size,
        uint32_t rate, uint32_t channels, int32_t bit_depth);

static struct pcm_config config;
static int configured;
static int volume = 100;
static vdevice_t sound_device;

static int sound_write(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        const void *buffer, int size, int offset, void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)offset; (void)data;
    if(!configured || size <= 0)
        return -1;
    return wasm_host_audio_write(buffer, (uint32_t)size, (uint32_t)config.rate,
            (uint32_t)config.channels, config.bit_depth);
}

static int sound_control(vdevice_t *dev, int from_pid, int cmd,
        proto_t *in, proto_t *out, void *data) {
    (void)dev; (void)from_pid; (void)data;
    switch(cmd) {
    case CTRL_PCM_DEV_HW:
        memset(&config, 0, sizeof(config));
        proto_read_to(in, &config, sizeof(config));
        configured = config.rate > 0 && config.channels > 0 &&
                (config.bit_depth == 8 || config.bit_depth == -8 ||
                 config.bit_depth == 16 || config.bit_depth == 32);
        return configured ? 0 : -1;
    case CTRL_PCM_DEV_HW_FREE:
        configured = 0;
        return 0;
    case CTRL_PCM_DEV_PRPARE:
        return configured ? 0 : -1;
    case CTRL_PCM_BUF_AVAIL:
        return configured ? config.period_size * config.channels *
                (config.bit_depth < 0 ? -config.bit_depth : config.bit_depth) / 8 : -1;
    case CTRL_SND_SET_VOLUME:
        volume = proto_read_int(in);
        return volume >= 0 && volume <= 100 ? 0 : -1;
    case CTRL_SND_GET_VOLUME:
        PF->addi(out, volume);
        return 0;
    default:
        return -1;
    }
}

int ewok_service_init(void) {
    memset(&sound_device, 0, sizeof(sound_device));
    strcpy(sound_device.desc, "browser WebAudio PCM");
    sound_device.write = sound_write;
    sound_device.dev_cntl = sound_control;
    return device_run(&sound_device, "/dev/sound0", FS_TYPE_CHAR, 0666, false);
}

int ewok_service_step(void) {
    return 0;
}
