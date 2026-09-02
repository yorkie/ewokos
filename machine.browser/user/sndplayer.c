#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD
#include <minimp3/minimp3.h>
#include <vorbis/vorbisfile.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <mouse/mouse.h>
#include <x/x.h>
#include <x/xwin.h>

#define CTRL_PCM_DEV_HW 0xF0
#define CTRL_PCM_DEV_HW_FREE 0xF1
#define CTRL_PCM_DEV_PRPARE 0xF2
#define WINDOW_WIDTH 520
#define WINDOW_HEIGHT 250
#define BAR_COUNT 24

extern int32_t wasm_host_launch_argument(char *buffer, uint32_t capacity);
extern uint32_t wasm_host_launch_generation(void);

struct pcm_config {
    int bit_depth;
    int rate;
    int channels;
    int period_size;
    int period_count;
    int start_threshold;
    int stop_threshold;
};

enum audio_format { AUDIO_NONE, AUDIO_MP3, AUDIO_WAV, AUDIO_OGG };

typedef struct {
    const uint8_t *data;
    uint32_t size;
    uint32_t offset;
} ogg_memory_t;

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static uint8_t *file_data;
static uint32_t file_size;
static uint32_t stream_offset;
static uint32_t wav_data_offset;
static uint32_t wav_data_size;
static uint32_t wav_bytes_per_frame;
static uint32_t current_frames;
static uint32_t total_frames;
static int sound_fd = -1;
static int sample_rate = 44100;
static int channels = 2;
static int playing;
static int eof;
static enum audio_format format;
static mp3dec_t decoder;
static OggVorbis_File ogg_file;
static ogg_memory_t ogg_memory;
static int ogg_opened;
static int ogg_bitstream;
static int16_t decode_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME];
static uint8_t bar_levels[BAR_COUNT];
static char current_path[256];
static char status_text[96] = "No audio loaded";
static uint32_t launch_generation;
static uint64_t playback_wall_ms;
static uint32_t playback_base_frames;

static size_t ogg_read(void *ptr, size_t size, size_t count, void *source) {
    ogg_memory_t *memory = source;
    if(size == 0 || count == 0 || memory->offset >= memory->size)
        return 0;
    uint64_t requested = (uint64_t)size * count;
    uint32_t available = memory->size - memory->offset;
    uint32_t copied = requested < available ? (uint32_t)requested : available;
    copied -= copied % size;
    memcpy(ptr, memory->data + memory->offset, copied);
    memory->offset += copied;
    return copied / size;
}

static int ogg_seek(void *source, ogg_int64_t offset, int whence) {
    ogg_memory_t *memory = source;
    int64_t next = whence == SEEK_SET ? offset :
            whence == SEEK_CUR ? (int64_t)memory->offset + offset :
            whence == SEEK_END ? (int64_t)memory->size + offset : -1;
    if(next < 0 || next > memory->size)
        return -1;
    memory->offset = (uint32_t)next;
    return 0;
}

static int ogg_close(void *source) {
    (void)source;
    return 0;
}

static long ogg_tell(void *source) {
    return (long)((ogg_memory_t *)source)->offset;
}

static int16_t float_to_s16(float sample) {
    if(sample > 1.0f) sample = 1.0f;
    if(sample < -1.0f) sample = -1.0f;
    return sample >= 0.0f ? (int16_t)(sample * 32767.0f + 0.5f) :
            (int16_t)(sample * 32768.0f - 0.5f);
}

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
            ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void close_sound(void) {
    if(sound_fd >= 0) {
        dev_cntl("/dev/sound0", CTRL_PCM_DEV_HW_FREE, NULL, NULL);
        close(sound_fd);
        sound_fd = -1;
    }
}

static int configure_sound(void) {
    close_sound();
    sound_fd = open("/dev/sound0", O_WRONLY);
    if(sound_fd < 0)
        return -1;
    struct pcm_config config = {
        16, sample_rate, channels, 2048, 4, 2048, 8192
    };
    proto_t in;
    PF->init(&in)->add(&in, &config, sizeof(config));
    int result = dev_cntl("/dev/sound0", CTRL_PCM_DEV_HW, &in, NULL);
    PF->clear(&in);
    if(result == 0)
        result = dev_cntl("/dev/sound0", CTRL_PCM_DEV_PRPARE, NULL, NULL);
    if(result != 0) {
        close_sound();
        return -1;
    }
    return 0;
}

static int read_file(const char *path) {
    int fd = open(path, O_RDONLY);
    if(fd < 0)
        return -1;
    const uint32_t capacity = 4u * 1024u * 1024u;
    uint8_t *data = malloc(capacity);
    if(data == NULL) {
        close(fd);
        return -1;
    }
    uint32_t total = 0;
    while(total < capacity) {
        uint32_t request = capacity - total;
        if(request > 32768)
            request = 32768;
        int count = read(fd, data + total, request);
        if(count <= 0)
            break;
        total += (uint32_t)count;
    }
    close(fd);
    if(total == 0 || total == capacity) {
        free(data);
        return -1;
    }
    free(file_data);
    file_data = data;
    file_size = total;
    return 0;
}

static int parse_wav(void) {
    if(file_size < 44 || memcmp(file_data, "RIFF", 4) != 0 ||
            memcmp(file_data + 8, "WAVE", 4) != 0)
        return -1;
    uint32_t offset = 12;
    int got_fmt = 0;
    while(offset + 8 <= file_size) {
        uint32_t size = read_u32(file_data + offset + 4);
        if(size > file_size - offset - 8)
            return -1;
        if(memcmp(file_data + offset, "fmt ", 4) == 0 && size >= 16) {
            if(read_u16(file_data + offset + 8) != 1 ||
                    read_u16(file_data + offset + 22) != 16)
                return -1;
            channels = read_u16(file_data + offset + 10);
            sample_rate = (int)read_u32(file_data + offset + 12);
            wav_bytes_per_frame = (uint32_t)channels * 2;
            got_fmt = channels > 0 && channels <= 2 && sample_rate > 0;
        }
        else if(memcmp(file_data + offset, "data", 4) == 0 && got_fmt) {
            wav_data_offset = offset + 8;
            wav_data_size = size;
            total_frames = size / wav_bytes_per_frame;
            stream_offset = wav_data_offset;
            return 0;
        }
        offset += 8 + size + (size & 1);
    }
    return -1;
}

static int probe_mp3(void) {
    mp3dec_init(&decoder);
    uint32_t offset = 0;
    mp3dec_frame_info_t info;
    while(offset < file_size) {
        int samples = mp3dec_decode_frame(&decoder, file_data + offset,
                (int)(file_size - offset), decode_buffer, &info);
        if(info.frame_bytes <= 0)
            return -1;
        offset += (uint32_t)info.frame_bytes;
        if(samples > 0) {
            sample_rate = info.hz;
            channels = info.channels;
            uint32_t bitrate = info.bitrate_kbps > 0 ?
                    (uint32_t)info.bitrate_kbps : 128u;
            total_frames = (uint32_t)(((uint64_t)file_size * 8u *
                    (uint32_t)sample_rate) / (bitrate * 1000u));
            stream_offset = 0;
            mp3dec_init(&decoder);
            return 0;
        }
    }
    return -1;
}

static int load_track(const char *path) {
    playing = 0;
    eof = 0;
    current_frames = 0;
    close_sound();
    if(ogg_opened) {
        ov_clear(&ogg_file);
        ogg_opened = 0;
    }
    if(read_file(path) != 0) {
        snprintf(status_text, sizeof(status_text), "Cannot open %.72s", path);
        return -1;
    }
    const char *extension = strrchr(path, '.');
    format = extension != NULL && strcasecmp(extension, ".wav") == 0 ?
            AUDIO_WAV : extension != NULL && strcasecmp(extension, ".ogg") == 0 ?
            AUDIO_OGG : AUDIO_MP3;
    int result;
    if(format == AUDIO_WAV)
        result = parse_wav();
    else if(format == AUDIO_MP3)
        result = probe_mp3();
    else {
        ogg_memory.data = file_data;
        ogg_memory.size = file_size;
        ogg_memory.offset = 0;
        ov_callbacks callbacks = {ogg_read, ogg_seek, ogg_close, ogg_tell};
        result = ov_open_callbacks(&ogg_memory, &ogg_file, NULL, 0, callbacks);
        if(result == 0) {
            vorbis_info *info = ov_info(&ogg_file, -1);
            if(info == NULL || info->channels < 1 || info->channels > 2) {
                ov_clear(&ogg_file);
                result = -1;
            }
            else {
                ogg_opened = 1;
                channels = info->channels;
                sample_rate = (int)info->rate;
                ogg_int64_t frames = ov_pcm_total(&ogg_file, -1);
                total_frames = frames > 0 && frames < UINT32_MAX ?
                        (uint32_t)frames : 0;
            }
        }
    }
    if(result != 0 || configure_sound() != 0) {
        snprintf(status_text, sizeof(status_text), "Unsupported audio: %.68s", path);
        format = AUDIO_NONE;
        return -1;
    }
    strncpy(current_path, path, sizeof(current_path) - 1);
    snprintf(status_text, sizeof(status_text), "%s · %d Hz · %d ch",
            format == AUDIO_WAV ? "PCM/WAV" :
            format == AUDIO_OGG ? "Ogg Vorbis" : "MP3", sample_rate, channels);
    memset(bar_levels, 0, sizeof(bar_levels));
    return 0;
}

static void update_bars(const int16_t *samples, int frames) {
    if(frames <= 0)
        return;
    for(int bar = 0; bar < BAR_COUNT; bar++) {
        int start = bar * frames / BAR_COUNT;
        int end = (bar + 1) * frames / BAR_COUNT;
        int peak = 0;
        for(int i = start; i < end; i++) {
            int value = samples[i * channels];
            if(value < 0)
                value = -value;
            if(value > peak)
                peak = value;
        }
        int level = peak * 100 / 32768;
        if(level > 100)
            level = 100;
        bar_levels[bar] = (uint8_t)level;
    }
}

static int decode_and_write(void) {
    if(format == AUDIO_MP3) {
        mp3dec_frame_info_t info;
        while(stream_offset < file_size) {
            int frames = mp3dec_decode_frame(&decoder,
                    file_data + stream_offset, (int)(file_size - stream_offset),
                    decode_buffer, &info);
            if(info.frame_bytes <= 0) {
                eof = 1;
                playing = 0;
                return 0;
            }
            stream_offset += (uint32_t)info.frame_bytes;
            if(frames <= 0)
                continue;
            if(write(sound_fd, decode_buffer,
                    (uint32_t)frames * (uint32_t)channels * 2u) < 0) {
                strcpy(status_text, "Audio device write failed");
                playing = 0;
                return -1;
            }
            current_frames += (uint32_t)frames;
            update_bars(decode_buffer, frames);
            return 1;
        }
    }
    else if(format == AUDIO_WAV && stream_offset < wav_data_offset + wav_data_size) {
        uint32_t remaining = wav_data_offset + wav_data_size - stream_offset;
        uint32_t frames = remaining / wav_bytes_per_frame;
        if(frames > 1152)
            frames = 1152;
        uint32_t bytes = frames * wav_bytes_per_frame;
        if(write(sound_fd, file_data + stream_offset, bytes) < 0) {
            strcpy(status_text, "Audio device write failed");
            playing = 0;
            return -1;
        }
        update_bars((const int16_t *)(file_data + stream_offset), (int)frames);
        stream_offset += bytes;
        current_frames += frames;
        return 1;
    }
    else if(format == AUDIO_OGG && ogg_opened) {
        float **pcm = NULL;
        long frames = ov_read_float(&ogg_file, &pcm, 1152, &ogg_bitstream);
        if(frames > 0) {
            for(int i = 0; i < frames; i++)
                for(int channel = 0; channel < channels; channel++)
                    decode_buffer[i * channels + channel] =
                            float_to_s16(pcm[channel][i]);
            if(write(sound_fd, decode_buffer,
                    (uint32_t)frames * (uint32_t)channels * 2u) < 0) {
                strcpy(status_text, "Audio device write failed");
                playing = 0;
                return -1;
            }
            current_frames += (uint32_t)frames;
            update_bars(decode_buffer, (int)frames);
            return 1;
        }
    }
    eof = 1;
    playing = 0;
    return 0;
}

static void seek_progress(uint32_t thousandths) {
    if(format == AUDIO_NONE)
        return;
    if(thousandths > 1000)
        thousandths = 1000;
    current_frames = (uint32_t)(((uint64_t)total_frames * thousandths) / 1000u);
    eof = 0;
    if(format == AUDIO_WAV) {
        stream_offset = wav_data_offset + (uint32_t)(((uint64_t)wav_data_size *
                thousandths) / 1000u);
        stream_offset -= (stream_offset - wav_data_offset) % wav_bytes_per_frame;
    }
    else if(format == AUDIO_OGG)
        ov_pcm_seek(&ogg_file, current_frames);
    else {
        stream_offset = (uint32_t)(((uint64_t)file_size * thousandths) / 1000u);
        mp3dec_init(&decoder);
    }
    playback_base_frames = current_frames;
    playback_wall_ms = kernel_tic_ms(0);
}

static const char *track_name(void) {
    const char *slash = strrchr(current_path, '/');
    return slash == NULL ? current_path : slash + 1;
}

static void text(graph_t *g, int x, int y, const char *value, uint32_t color) {
    graph_draw_text_font(g, x, y, value, app_font, app_theme.fontSize, color);
}

static void button(graph_t *g, int x, int y, int w, const char *label,
        uint32_t color) {
    graph_fill_round(g, x, y, w, 30, 7, color);
    graph_round(g, x, y, w, 30, 7, 1, 0x88ffffffu);
    graph_draw_text_font_align(g, x, y + 2, w, 26, label, app_font,
            16, 0xffffffffu, FONT_ALIGN_CENTER);
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xff161625u);
    graph_fill_rect(g, 0, 0, g->w, 42, 0xff202b35u);
    text(g, 14, 10, current_path[0] ? track_name() : "SndPlayer.wasm",
            0xffffffffu);
    text(g, 14, 27, status_text, 0xff9fb0bdu);

    int spectrum_top = 52;
    int spectrum_height = 105;
    graph_fill_round(g, 12, spectrum_top, g->w - 24, spectrum_height,
            8, 0xff101820u);
    int bar_w = (g->w - 44) / BAR_COUNT;
    for(int i = 0; i < BAR_COUNT; i++) {
        int height = 4 + bar_levels[i] * (spectrum_height - 16) / 100;
        uint32_t color = 0xff00aaeeu |
                ((uint32_t)(bar_levels[i] * 2) << 16);
        graph_fill_rect(g, 22 + i * bar_w,
                spectrum_top + spectrum_height - height - 6,
                bar_w - 2, height, color);
    }

    int progress_x = 14;
    int progress_y = 170;
    int progress_w = g->w - 28;
    graph_fill_round(g, progress_x, progress_y, progress_w, 14, 7,
            0xff38434du);
    uint32_t progress = total_frames > 0 ?
            (uint32_t)(((uint64_t)current_frames * 1000u) / total_frames) : 0;
    if(progress > 1000)
        progress = 1000;
    graph_fill_round(g, progress_x, progress_y,
            progress_w * (int)progress / 1000, 14, 7, 0xff00aaffu);
    uint32_t current_seconds = sample_rate > 0 ? current_frames / sample_rate : 0;
    uint32_t total_seconds = sample_rate > 0 ? total_frames / sample_rate : 0;
    char time_value[48];
    snprintf(time_value, sizeof(time_value), "%02u:%02u / %02u:%02u",
            current_seconds / 60, current_seconds % 60,
            total_seconds / 60, total_seconds % 60);
    text(g, 14, 194, time_value, 0xffd7e3eau);
    button(g, 260, 198, 72, playing ? "Pause" : "Play", 0xff315c7au);
    button(g, 342, 198, 72, "Stop", 0xff7a3f48u);
    button(g, 424, 198, 82, "Default", 0xff2f7d55u);
}

static int inside(gpos_t p, int x, int y, int w, int h) {
    return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type != XEVT_MOUSE || event_value->state != MOUSE_STATE_CLICK)
        return;
    gpos_t p = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);
    if(inside(p, 14, 166, window->xinfo->wsr.w - 28, 24)) {
        uint32_t progress = (uint32_t)(p.x - 14) * 1000u /
                (uint32_t)(window->xinfo->wsr.w - 28);
        seek_progress(progress);
    }
    else if(inside(p, 260, 198, 72, 30) && format != AUDIO_NONE) {
        if(eof)
            seek_progress(0);
        playing = !playing;
        if(playing) {
            playback_base_frames = current_frames;
            playback_wall_ms = kernel_tic_ms(0);
        }
    }
    else if(inside(p, 342, 198, 72, 30)) {
        playing = 0;
        seek_progress(0);
        memset(bar_levels, 0, sizeof(bar_levels));
    }
    else if(inside(p, 424, 198, 82, 30))
        load_track("/usr/system/sounds/start.mp3");
    xwin_repaint(window);
}

static int refresh_launch_argument(void) {
    uint32_t generation = wasm_host_launch_generation();
    if(generation == launch_generation)
        return 0;
    launch_generation = generation;
    char argument[256] = {0};
    wasm_host_launch_argument(argument, sizeof(argument));
    if(argument[0] == 0)
        return 0;
    load_track(argument);
    return 1;
}

static int open_app_window(void) {
    if(app_window != NULL && app_window->fd > 0 && app_window->xinfo != NULL)
        return 0;
    if(app_window != NULL)
        xwin_destroy(app_window);
    app_x.main_win = NULL;
    app_x.terminated = false;
    app_window = xwin_open(&app_x, -1, 180, 120, WINDOW_WIDTH, WINDOW_HEIGHT,
            "SndPlayer.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/SndPlayer/SndPlayer");
    xwin_set_visible(app_window, true);
    return 0;
}

void ewok_launch_argument_changed(void) {
    refresh_launch_argument();
    if(open_app_window() == 0) {
        xwin_top(app_window);
        xwin_repaint(app_window);
    }
}

int ewok_service_init(void) {
    memset(&app_x, 0, sizeof(app_x));
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    if(app_font == NULL || open_app_window() != 0)
        return -1;
    refresh_launch_argument();
    if(format == AUDIO_NONE)
        load_track("/usr/system/sounds/start.mp3");
    xwin_repaint(app_window);
    return 0;
}

int ewok_service_step(void) {
    if(app_window != NULL && app_window->fd > 0) {
        for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
        if(playing) {
            uint64_t elapsed = kernel_tic_ms(0) - playback_wall_ms;
            uint64_t target = playback_base_frames +
                    elapsed * (uint32_t)sample_rate / 1000u +
                    (uint32_t)sample_rate / 20u;
            if(current_frames < target && decode_and_write() > 0)
                xwin_repaint(app_window);
        }
    }
    return 0;
}
