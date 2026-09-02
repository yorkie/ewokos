#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>

#define CTRL_PCM_DEV_HW 0xF0
#define CTRL_PCM_DEV_HW_FREE 0xF1

struct pcm_config {
    int bit_depth;
    int rate;
    int channels;
    int period_size;
    int period_count;
    int start_threshold;
    int stop_threshold;
};

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    int fd = open("/dev/sound0", O_WRONLY);
    if(fd < 0)
        return -1;
    struct pcm_config config = {16, 22050, 1, 512, 4, 0, 0};
    proto_t in;
    PF->init(&in)->add(&in, &config, sizeof(config));
    int result = dev_cntl_by_pid(dev_get_pid("/dev/sound0"),
            CTRL_PCM_DEV_HW, &in, NULL);
    PF->clear(&in);
    if(result != 0)
        return -1;

    int16_t samples[2205];
    int32_t phase = 0;
    for(uint32_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        phase = (phase + 440) % 22050;
        samples[i] = phase < 11025 ? 3500 : -3500;
    }
    result = write(fd, samples, sizeof(samples));
    dev_cntl_by_pid(dev_get_pid("/dev/sound0"), CTRL_PCM_DEV_HW_FREE, NULL, NULL);
    close(fd);
    if(result != (int)sizeof(samples))
        return -1;
    klog("sound.wasm: delivered 2205 PCM samples to WebAudio\n");
    return 0;
}
