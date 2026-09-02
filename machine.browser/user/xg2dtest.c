#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ewoksys/kernel_tic.h>
#include <font/font.h>
#include <g2dclient/g2dclient.h>
#include <graph/graph_ex.h>
#include <x/x.h>
#include <x/xwin.h>

#define CANVAS_W 560u
#define CANVAS_H 320u
#define MAX_LOGS 28
#define LOG_SIZE 112
#define BENCH_FRAMES 12u

typedef struct {
    graph_t *graph;
    uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t size;
} shm_image_t;

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static graph_t *preview;
static char logs[MAX_LOGS][LOG_SIZE];
static uint32_t log_count;
static uint32_t failures;
static uint32_t fps;
static uint32_t usec_per_frame;
static int test_done;

static void append_log(const char *format, ...) {
    if(log_count >= MAX_LOGS)
        return;
    va_list values;
    va_start(values, format);
    vsnprintf(logs[log_count], LOG_SIZE, format, values);
    va_end(values);
    log_count++;
}

static void fail(const char *label) {
    failures++;
    append_log("FAIL  %s", label);
}

static void check_result(const char *label, int result, int expect_success) {
    int passed = expect_success ? result == 0 : result != 0;
    if(passed)
        append_log("PASS  %-20s ret=%d", label, result);
    else
        fail(label);
}

static void check_pixel(const char *label, const shm_image_t *image,
        uint32_t x, uint32_t y, uint32_t expected) {
    if(image == NULL || image->pixels == NULL || x >= image->width ||
            y >= image->height ||
            image->pixels[y * image->width + x] != expected) {
        fail(label);
        return;
    }
    append_log("PASS  %-20s (%u,%u)", label, x, y);
}

static int image_create(shm_image_t *image, uint32_t width, uint32_t height) {
    memset(image, 0, sizeof(*image));
    image->graph = graph_new_shm((int32_t)width, (int32_t)height);
    if(image->graph == NULL || image->graph->buffer == NULL)
        return -1;
    image->pixels = image->graph->buffer;
    image->width = width;
    image->height = height;
    image->size = width * height * sizeof(uint32_t);
    return 0;
}

static void image_destroy(shm_image_t *image) {
    if(image->graph != NULL)
        graph_free(image->graph);
    memset(image, 0, sizeof(*image));
}

static g2d_canvas_t image_canvas(const shm_image_t *image) {
    return g2d_canvas(image->graph->shm_id, image->size,
            image->width, image->height,
            image->graph->shm_contig ? 1 : 0);
}

static void image_clear(shm_image_t *image, uint32_t color) {
    uint32_t count = image->width * image->height;
    for(uint32_t i = 0; i < count; i++)
        image->pixels[i] = color;
}

static void fill_checker(shm_image_t *image) {
    for(uint32_t y = 0; y < image->height; y++) {
        for(uint32_t x = 0; x < image->width; x++) {
            uint32_t r = x * 255 / image->width;
            uint32_t g = y * 255 / image->height;
            uint32_t b = ((x / 16 + y / 16) & 1) ? 0xd0 : 0x30;
            image->pixels[y * image->width + x] =
                    0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
}

static void fill_alpha_circle(shm_image_t *image) {
    int32_t cx = (int32_t)image->width / 2;
    int32_t cy = (int32_t)image->height / 2;
    int32_t radius = (int32_t)(image->width < image->height ?
            image->width : image->height) / 2 - 2;
    int32_t radius2 = radius * radius;
    for(int32_t y = 0; y < (int32_t)image->height; y++) {
        for(int32_t x = 0; x < (int32_t)image->width; x++) {
            int32_t dx = x - cx;
            int32_t dy = y - cy;
            int32_t distance = dx * dx + dy * dy;
            uint32_t alpha = distance < radius2 ?
                    (uint32_t)(255 - distance * 255 / radius2) : 0;
            image->pixels[y * image->width + x] =
                    (alpha << 24) | 0x00ffe020u;
        }
    }
}

static void set_corner_markers(shm_image_t *image) {
    uint32_t width = image->width;
    uint32_t height = image->height;
    image->pixels[0] = 0xff000001u;
    image->pixels[width - 1] = 0xff000002u;
    image->pixels[(height - 1) * width + width - 1] = 0xff000003u;
    image->pixels[(height - 1) * width] = 0xff000004u;
}

static void publish_preview(const shm_image_t *image) {
    graph_t wrapped;
    graph_init(&wrapped, image->pixels, (int32_t)image->width,
            (int32_t)image->height);
    graph_t *copy = graph_dup(&wrapped);
    if(copy == NULL)
        return;
    if(preview != NULL)
        graph_free(preview);
    preview = copy;
}

static uint32_t now_usec(void) {
    uint32_t value = 0;
    kernel_tic32(NULL, NULL, &value);
    return value;
}

static void run_benchmark(void) {
    shm_image_t canvas;
    shm_image_t opaque;
    shm_image_t alpha;
    memset(&canvas, 0, sizeof(canvas));
    memset(&opaque, 0, sizeof(opaque));
    memset(&alpha, 0, sizeof(alpha));
    if(image_create(&canvas, 800, 600) != 0 ||
            image_create(&opaque, 640, 480) != 0 ||
            image_create(&alpha, 640, 480) != 0) {
        fail("benchmark canvases");
        goto cleanup;
    }
    fill_checker(&opaque);
    fill_alpha_circle(&alpha);
    uint32_t start = now_usec();
    uint32_t completed = 0;
    for(uint32_t i = 0; i < BENCH_FRAMES; i++) {
        g2d_fill_req_t fill;
        g2d_blit_req_t blit;
        g2d_fill_req_init(&fill, image_canvas(&canvas),
                g2d_rect(0, 0, 800, 600), 0xff000000u | i * 31u);
        if(g2d_fill_rect(&fill) != 0)
            break;
        g2d_blit_req_init(&blit, image_canvas(&canvas), image_canvas(&opaque),
                g2d_rect(0, 0, 640, 480), g2d_rect(0, 0, 800, 600), 0xff);
        if(g2d_blit(&blit) != 0)
            break;
        g2d_blit_req_init(&blit, image_canvas(&canvas), image_canvas(&alpha),
                g2d_rect(0, 0, 640, 480), g2d_rect(0, 0, 800, 600), 0xff);
        if(g2d_blit_alpha(&blit) != 0)
            break;
        completed++;
    }
    uint32_t elapsed = now_usec() - start;
    if(completed > 0 && elapsed > 0) {
        fps = (uint32_t)(((uint64_t)completed * 1000000u) / elapsed);
        usec_per_frame = elapsed / completed;
        append_log("BENCH %u frames: %u fps", completed, fps);
    }
    else {
        fail("benchmark frame");
    }

cleanup:
    image_destroy(&alpha);
    image_destroy(&opaque);
    image_destroy(&canvas);
}

static void run_test(void) {
    shm_image_t canvas_a;
    shm_image_t canvas_b;
    shm_image_t canvas_c;
    shm_image_t opaque;
    shm_image_t alpha;
    shm_image_t scaled;
    memset(&canvas_a, 0, sizeof(canvas_a));
    memset(&canvas_b, 0, sizeof(canvas_b));
    memset(&canvas_c, 0, sizeof(canvas_c));
    memset(&opaque, 0, sizeof(opaque));
    memset(&alpha, 0, sizeof(alpha));
    memset(&scaled, 0, sizeof(scaled));
    log_count = 0;
    failures = 0;

    if(has_g2d() != 0) {
        fail("/dev/g2d available");
        goto cleanup;
    }
    append_log("g2d shared-memory API %ux%u", CANVAS_W, CANVAS_H);
    if(image_create(&canvas_a, CANVAS_W, CANVAS_H) != 0 ||
            image_create(&canvas_b, CANVAS_H, CANVAS_W) != 0 ||
            image_create(&canvas_c, CANVAS_H, CANVAS_W) != 0 ||
            image_create(&opaque, 160, 120) != 0 ||
            image_create(&alpha, 128, 128) != 0) {
        fail("create shared canvases");
        goto cleanup;
    }
    fill_checker(&opaque);
    fill_alpha_circle(&alpha);

    const uint32_t background = 0xff101820u;
    g2d_fill_req_t fill;
    g2d_blit_req_t blit;
    g2d_rotate_req_t rotate;
    g2d_scale_to_req_t scale;
    image_clear(&canvas_a, background);
    g2d_fill_req_init(&fill, image_canvas(&canvas_a),
            g2d_rect(24, 24, 220, 120), 0xff204060u);
    check_result("fill_rect", g2d_fill_rect(&fill), 1);
    check_pixel("fill_inside", &canvas_a, 100, 60, 0xff204060u);
    check_pixel("fill_outside", &canvas_a, 10, 10, background);

    g2d_blit_req_init(&blit, image_canvas(&canvas_a), image_canvas(&opaque),
            g2d_rect(0, 0, 160, 120), g2d_rect(48, 172, 160, 120), 0xff);
    check_result("blit_opaque", g2d_blit(&blit), 1);
    check_pixel("blit_pixel", &canvas_a, 48, 172, opaque.pixels[0]);

    g2d_blit_req_init(&blit, image_canvas(&canvas_a), image_canvas(&opaque),
            g2d_rect(0, 0, 160, 120), g2d_rect(280, 160, 200, 130), 0xff);
    check_result("blit_scale", g2d_blit(&blit), 1);
    check_pixel("blit_scale_tl", &canvas_a, 280, 160, opaque.pixels[0]);

    g2d_blit_req_init_ex(&blit, image_canvas(&canvas_a), image_canvas(&opaque),
            g2d_rect(20, 16, 80, 60), g2d_rect(232, 24, 120, 100),
            0xff, G2D_ROTATE_90);
    check_result("blit_rotate_90", g2d_blit(&blit), 1);
    check_pixel("blit_rot90_tl", &canvas_a, 232, 24,
            opaque.pixels[75 * opaque.width + 20]);

    g2d_blit_req_init(&blit, image_canvas(&canvas_a), image_canvas(&alpha),
            g2d_rect(0, 0, 128, 128), g2d_rect(220, 150, 128, 128), 0xff);
    check_result("blit_alpha", g2d_blit_alpha(&blit), 1);
    check_pixel("alpha_corner", &canvas_a, 220, 150, background);
    publish_preview(&canvas_a);

    image_clear(&canvas_a, background);
    set_corner_markers(&canvas_a);
    g2d_rotate_req_init(&rotate, image_canvas(&canvas_a),
            image_canvas(&canvas_b), 90);
    check_result("rotate_90", g2d_rotate(&rotate), 1);
    check_pixel("rot90_TL_from_BL", &canvas_b, 0, 0, 0xff000004u);
    check_pixel("rot90_TR_from_TL", &canvas_b,
            canvas_b.width - 1, 0, 0xff000001u);

    g2d_rotate_req_init(&rotate, image_canvas(&canvas_b),
            image_canvas(&canvas_c), 180);
    check_result("rotate_180", g2d_rotate(&rotate), 1);
    check_pixel("rot180_TL", &canvas_c, 0, 0, 0xff000002u);

    g2d_rotate_req_init(&rotate, image_canvas(&canvas_c),
            image_canvas(&canvas_a), 270);
    check_result("rotate_270", g2d_rotate(&rotate), 1);
    check_pixel("rot270_TL", &canvas_a, 0, 0, 0xff000003u);
    check_pixel("rot270_BR", &canvas_a, canvas_a.width - 1,
            canvas_a.height - 1, 0xff000001u);

    g2d_rotate_req_init(&rotate, image_canvas(&canvas_a),
            image_canvas(&canvas_a), 90);
    check_result("rotate_bad_size", g2d_rotate(&rotate), 0);
    g2d_rotate_req_init(&rotate, image_canvas(&canvas_a),
            image_canvas(&canvas_b), 0);
    check_result("rotate_zero_reject", g2d_rotate(&rotate), 0);

    fill_checker(&canvas_a);
    if(image_create(&scaled, 320, 240) == 0) {
        g2d_scale_to_req_init(&scale, image_canvas(&canvas_a),
                image_canvas(&scaled));
        check_result("scale_to_320x240", g2d_scale_to(&scale), 1);
        check_pixel("scale_top_left", &scaled, 0, 0, canvas_a.pixels[0]);
        check_pixel("scale_bottom_right", &scaled, 319, 239,
                canvas_a.pixels[canvas_a.width * canvas_a.height - 1]);
    }
    else {
        fail("scale canvas");
    }

    image_clear(&canvas_a, background);
    g2d_blit_req_init_ex(&blit, image_canvas(&canvas_a), image_canvas(&opaque),
            g2d_rect(0, 0, 160, 120), g2d_rect(170, 50, 220, 220), 0xff, 45);
    check_result("blit_rotate_45", g2d_blit(&blit), 1);
    publish_preview(&canvas_a);
    run_benchmark();

cleanup:
    image_destroy(&scaled);
    image_destroy(&alpha);
    image_destroy(&opaque);
    image_destroy(&canvas_c);
    image_destroy(&canvas_b);
    image_destroy(&canvas_a);
    test_done = 1;
}

static void repaint(xwin_t *window, graph_t *graph) {
    (void)window;
    int header_height = 54;
    int right_width = graph->w / 3;
    int left_width = graph->w - right_width;
    uint32_t header = !test_done ? 0xff3a5878u :
            failures == 0 ? 0xff1d7f3bu : 0xff8f2d2du;
    graph_fill_rect(graph, 0, 0, graph->w, graph->h, 0xff1e1e24u);
    graph_fill_rect(graph, 0, 0, graph->w, header_height, header);
    char status[160];
    snprintf(status, sizeof(status), "xg2dtest %s · %u fps · %u us/frame",
            !test_done ? "RUNNING" : failures == 0 ? "TEST PASSED" : "TEST FAILED",
            fps, usec_per_frame);
    graph_draw_text_font(graph, 12, 8, status, app_font,
            app_theme.fontSize, 0xffffffffu);
    graph_draw_text_font(graph, 12, 29,
            "Native EwokOS /dev/g2d shared-memory validation",
            app_font, app_theme.fontSize, 0xffffffffu);

    int preview_x = 12;
    int preview_y = header_height + 12;
    int preview_width = left_width - 24;
    int preview_height = graph->h - header_height - 24;
    graph_fill_rect(graph, preview_x - 1, preview_y - 1,
            preview_width + 2, preview_height + 2, 0xff555566u);
    graph_fill_rect(graph, preview_x, preview_y,
            preview_width, preview_height, 0xff0f0f14u);
    if(preview != NULL)
        graph_blt_fit(preview, 0, 0, preview->w, preview->h, graph,
                preview_x, preview_y, preview_width, preview_height);

    graph_fill_rect(graph, left_width, header_height, right_width,
            graph->h - header_height, 0xff16161cu);
    graph_set_clip(graph, left_width, header_height, right_width,
            graph->h - header_height);
    for(uint32_t i = 0; i < log_count; i++) {
        int y = header_height + 10 + (int)i * (app_theme.fontSize + 3);
        if(y + (int)app_theme.fontSize >= graph->h)
            break;
        graph_draw_text_font(graph, left_width + 10, y, logs[i], app_font,
                app_theme.fontSize, 0xffe8e8e8u);
    }
    graph_unset_clip(graph);
}

static int open_app_window(void) {
    if(app_window != NULL && app_window->fd > 0 && app_window->xinfo != NULL)
        return 0;
    if(app_window != NULL)
        xwin_destroy(app_window);
    app_x.main_win = NULL;
    app_x.terminated = false;
    app_window = xwin_open(&app_x, -1, 36, 62, 920, 540,
            "xg2dtest.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    x_set_app_name(&app_x, "/apps/xg2dtest/xg2dtest");
    xwin_set_visible(app_window, true);
    return 0;
}

void ewok_launch_argument_changed(void) {
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
    run_test();
    xwin_repaint(app_window);
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return -1;
    if(app_window->fd > 0)
        for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    return 0;
}
