#include <stdint.h>
#include <display/display.h>

static int32_t display_probe_stage;
int32_t ewok_display_probe_stage(void) { return display_probe_stage; }

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    display_t display;
    display_probe_stage = 1;
    if(display_open("/dev/disp0", 0, &display) != 0)
        return -1;
    display_probe_stage = 2;
    graph_t *graph = display_fetch_graph(&display);
    if(graph == NULL || graph->buffer == NULL)
        return -1;

    display_probe_stage = 3;
    for(int32_t y = 0; y < graph->h; y++) {
        for(int32_t x = 0; x < graph->w; x++) {
            uint32_t red = (uint32_t)x * 0x80u /
                (uint32_t)(graph->w > 1 ? graph->w - 1 : 1);
            uint32_t blue = (uint32_t)y * 0x80u /
                (uint32_t)(graph->h > 1 ? graph->h - 1 : 1);
            uint32_t green = ((x / 32 + y / 32) & 1) ? 0x50u : 0x20u;
            graph->buffer[y * graph->w + x] =
                0xff000000u | (red << 16) | (green << 8) | blue;
        }
    }

    grect_t dirty = { 0, 0, graph->w, graph->h };
    display_set_dirty(&display, &dirty, 1);
    display_probe_stage = 4;
    if(display_flush(&display, false) != 0)
        return -1;
    display_probe_stage = 5;
    return 0;
}
