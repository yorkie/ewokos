#include <stdint.h>

#include <ewoksys/klog.h>
#include <g2dclient/g2dclient.h>

int ewok_g2d_probe_result = -1;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uint32_t *pixels = NULL;
    int shm_id = -1;
    const uint32_t color = 0xff2a7fff;
    if(g2d_shm_alloc(8 * 8 * sizeof(uint32_t), &shm_id, &pixels) != 0)
        return -1;
    for(int i = 0; i < 64; i++)
        pixels[i] = 0;
    g2d_fill_req_t request;
    g2d_fill_req_init(&request,
            g2d_canvas(shm_id, 8 * 8 * sizeof(uint32_t), 8, 8, 0),
            g2d_rect(1, 1, 6, 6), color);
    if(g2d_fill_rect(&request) != 0) {
        g2d_shm_free(pixels);
        return -1;
    }
    if(pixels[0] != 0 || pixels[1 + 1 * 8] != color ||
            pixels[6 + 6 * 8] != color || pixels[7 + 7 * 8] != 0) {
        g2d_shm_free(pixels);
        return -1;
    }
    uint32_t *src = NULL;
    int src_id = -1;
    if(g2d_shm_alloc(4 * 4 * sizeof(uint32_t), &src_id, &src) != 0) {
        g2d_shm_free(pixels);
        return -1;
    }
    src[0] = 0xffff0000; src[1] = 0xff00ff00;
    src[2] = 0xff0000ff; src[3] = 0xffffffff;
    for(int i = 0; i < 64; i++) pixels[i] = 0;
    g2d_blit_req_t blit;
    g2d_blit_req_init(&blit,
            g2d_canvas(shm_id, 8 * 8 * sizeof(uint32_t), 8, 8, 0),
            g2d_canvas(src_id, 4 * 4 * sizeof(uint32_t), 2, 2, 0),
            g2d_rect(0, 0, 2, 2), g2d_rect(0, 0, 4, 4), 0xff);
    if(g2d_blit(&blit) != 0 || pixels[0] != src[0] ||
            pixels[3] != src[1] || pixels[3 * 8] != src[2] || pixels[3 * 8 + 3] != src[3])
        goto failed;

    for(int i = 0; i < 16; i++) pixels[i] = 0;
    g2d_scale_to_req_t scale;
    g2d_scale_to_req_init(&scale,
            g2d_canvas(src_id, 4 * 4 * sizeof(uint32_t), 2, 2, 0),
            g2d_canvas(shm_id, 8 * 8 * sizeof(uint32_t), 4, 4, 0));
    if(g2d_scale_to(&scale) != 0 || pixels[0] != src[0] ||
            pixels[3] != src[1] || pixels[12] != src[2] || pixels[15] != src[3])
        goto failed;

    for(int i = 0; i < 16; i++) src[i] = 0xff000000u | (uint32_t)i;
    for(int i = 0; i < 4; i++) pixels[i] = 0;
    g2d_scale_to_req_init(&scale,
            g2d_canvas(src_id, 4 * 4 * sizeof(uint32_t), 4, 4, 0),
            g2d_canvas(shm_id, 8 * 8 * sizeof(uint32_t), 2, 2, 0));
    if(g2d_scale_to(&scale) != 0 || pixels[0] != src[0] ||
            pixels[1] != src[3] || pixels[2] != src[12] || pixels[3] != src[15])
        goto failed;

    src[0] = 0x80ff0000;
    pixels[0] = 0xff000000;
    g2d_blit_req_init(&blit,
            g2d_canvas(shm_id, 8 * 8 * sizeof(uint32_t), 1, 1, 0),
            g2d_canvas(src_id, 4 * 4 * sizeof(uint32_t), 1, 1, 0),
            g2d_rect(0, 0, 1, 1), g2d_rect(0, 0, 1, 1), 0x80);
    if(g2d_blit_alpha(&blit) != 0 || pixels[0] != 0xff400000)
        goto failed;

    src[0] = 1; src[1] = 2;
    src[2] = 3; src[3] = 4;
    src[4] = 5; src[5] = 6;
    g2d_rotate_req_t rotate;
    g2d_rotate_req_init(&rotate,
            g2d_canvas(src_id, 4 * 4 * sizeof(uint32_t), 2, 3, 0),
            g2d_canvas(shm_id, 8 * 8 * sizeof(uint32_t), 3, 2, 0), 90);
    if(g2d_rotate(&rotate) != 0 || pixels[0] != 5 || pixels[1] != 3 ||
            pixels[2] != 1 || pixels[3] != 6 || pixels[4] != 4 || pixels[5] != 2)
        goto failed;

    g2d_shm_free(src);
    g2d_shm_free(pixels);
    ewok_g2d_probe_result = 0;
    klog("g2d_probe.wasm: fill, blit, alpha, scale and rotate passed\n");
    return 0;

failed:
    g2d_shm_free(src);
    g2d_shm_free(pixels);
    return -1;
}
