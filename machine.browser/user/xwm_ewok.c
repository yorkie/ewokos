#include <stdint.h>
#include <string.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <graph/graph_image.h>
#include <ewoksys/klog.h>
#include <ewoksys/proto.h>
#include <x/xwm.h>

static xwm_t wm;
static font_t *font;
static graph_t *desktop_pattern;
static graph_t *round_mask;
static int round_mask_size;

static void reload_theme_assets(void) {
    if(font != NULL)
        font_free(font);
    font = font_new(wm.theme.fontName, true);
    if(desktop_pattern != NULL)
        graph_free(desktop_pattern);
    desktop_pattern = NULL;
    if(wm.theme.patternName[0] != 0)
        desktop_pattern = graph_image_new_bg(wm.theme.patternName,
                wm.theme.desktopBGColor);
    if(round_mask != NULL) {
        graph_free(round_mask);
        round_mask = NULL;
        round_mask_size = 0;
    }
}

static void update_theme(bool load_from_x, void *data) {
    (void)load_from_x;
    (void)data;
    reload_theme_assets();
}

static void fill_corner_mask(graph_t *mask, int cx, int cy, int radius) {
    int radius8 = radius * 8;
    int radius_sq = radius8 * radius8;
    for(int y = 0; y < mask->h; y++) {
        for(int x = 0; x < mask->w; x++) {
            int covered = 0;
            for(int sy = 1; sy < 8; sy += 2) {
                int dy = y * 8 + sy - cy * 8;
                for(int sx = 1; sx < 8; sx += 2) {
                    int dx = x * 8 + sx - cx * 8;
                    if(dx * dx + dy * dy <= radius_sq)
                        covered++;
                }
            }
            uint32_t alpha = (uint32_t)(covered * 255 / 16);
            mask->buffer[y * mask->w + x] =
                    (alpha << 24) | (alpha == 0 ? 0 : 0x00ffffffu);
        }
    }
}

static void get_win_space(int style, int state, grect_t *workspace,
        grect_t *window, void *data) {
    (void)data;
    *window = *workspace;
    int edge = state == XWIN_STATE_MAX || state == XWIN_STATE_FULL_SCREEN;
    int frame = edge ? 0 : (int)wm.theme.frameW;
    int shadow = edge ? 0 : (int)wm.theme.shadow;
    if((style & XWIN_STYLE_NO_TITLE) == 0 &&
            (style & XWIN_STYLE_NO_FRAME) == 0 &&
            state != XWIN_STATE_FULL_SCREEN) {
        window->y -= wm.theme.titleH;
        window->h += wm.theme.titleH;
    }
    if((style & XWIN_STYLE_NO_FRAME) == 0) {
        window->x -= frame;
        window->y -= frame;
        window->w += frame * 2 + shadow;
        window->h += frame * 2 + shadow;
    }
}

static int frame_width(const xinfo_t *info) {
    return info->state == XWIN_STATE_MAX ||
            info->state == XWIN_STATE_FULL_SCREEN ? 0 : (int)wm.theme.frameW;
}

static void get_title(xinfo_t *info, grect_t *r, void *data) {
    (void)data;
    int fw = frame_width(info);
    r->x = fw; r->y = fw;
    r->w = info->winr.w - fw * 2 - wm.theme.shadow;
    r->h = wm.theme.titleH;
}

static void get_close(xinfo_t *info, grect_t *r, void *data) {
    (void)data;
    int fw = frame_width(info);
    r->x = fw; r->y = fw; r->w = r->h = wm.theme.titleH;
}

static void get_max(xinfo_t *info, grect_t *r, void *data) {
    (void)data;
    int fw = frame_width(info);
    r->x = info->winr.w - wm.theme.titleH - fw - wm.theme.shadow;
    r->y = fw; r->w = r->h = wm.theme.titleH;
}

static void get_min(xinfo_t *info, grect_t *r, void *data) {
    (void)data;
    int fw = frame_width(info);
    r->x = info->winr.w - wm.theme.titleH * 2 - fw - wm.theme.shadow;
    r->y = fw; r->w = r->h = wm.theme.titleH;
}

static void get_resize(xinfo_t *info, grect_t *r, void *data) {
    (void)data;
    int fw = frame_width(info);
    r->x = info->winr.w - 20 - fw - wm.theme.shadow;
    r->y = info->winr.h - 20 - fw - wm.theme.shadow;
    r->w = r->h = 20 + fw;
}

static void get_frame(xinfo_t *info, grect_t *r, void *data) {
    (void)data;
    r->x = r->y = 0;
    r->w = info->winr.w - wm.theme.shadow;
    r->h = info->winr.h - wm.theme.shadow;
}

static void get_min_size(xinfo_t *info, int *w, int *h, void *data) {
    (void)info; (void)data;
    *w = wm.theme.titleH * 5;
    *h = wm.theme.titleH * 2;
}

static void draw_desktop(graph_t *g, void *data) {
    (void)data;
    graph_clear(g, wm.theme.desktopBGColor);
    if(desktop_pattern != NULL) {
        /* Fill the desktop without changing the wallpaper aspect ratio.
         * Crop the source symmetrically when its aspect differs from the
         * framebuffer, matching a conventional "cover" wallpaper mode. */
        int sx = 0;
        int sy = 0;
        int sw = desktop_pattern->w;
        int sh = desktop_pattern->h;
        if((int64_t)sw * g->h > (int64_t)g->w * sh) {
            int crop_w = (int)((int64_t)sh * g->w / g->h);
            sx = (sw - crop_w) / 2;
            sw = crop_w;
        }
        else if((int64_t)sw * g->h < (int64_t)g->w * sh) {
            int crop_h = (int)((int64_t)sw * g->h / g->w);
            sy = (sh - crop_h) / 2;
            sh = crop_h;
        }
        graph_blt_fit(desktop_pattern, sx, sy, sw, sh,
                g, 0, 0, g->w, g->h);
    }
}

static void draw_frame(graph_t *desktop, graph_t *frame, graph_t *workspace,
        xinfo_t *info, grect_t *r, bool top, void *data) {
    (void)desktop; (void)workspace; (void)info; (void)data;
    int radius = (int)wm.theme.round;
    if(radius > 0) {
        if(round_mask == NULL || round_mask_size != radius) {
            if(round_mask != NULL)
                graph_free(round_mask);
            round_mask = graph_new(NULL, radius, radius);
            round_mask_size = round_mask == NULL ? 0 : radius;
        }
        if(round_mask != NULL) {
            fill_corner_mask(round_mask, radius, radius, radius);
            graph_blt_alpha_mask(round_mask, 0, 0, radius, radius,
                    frame, 1, 1, radius, radius);

            fill_corner_mask(round_mask, radius, 0, radius);
            graph_blt_alpha_mask(round_mask, 0, 0, radius, radius,
                    frame, 1, frame->h - radius - 1, radius, radius);

            fill_corner_mask(round_mask, 0, radius, radius);
            graph_blt_alpha_mask(round_mask, 0, 0, radius, radius,
                    frame, frame->w - radius - 1, 1, radius, radius);

            fill_corner_mask(round_mask, 0, 0, radius);
            graph_blt_alpha_mask(round_mask, 0, 0, radius, radius,
                    frame, frame->w - radius - 1,
                    frame->h - radius - 1, radius, radius);
        }
    }
    uint32_t color = top ? wm.theme.frameBGColor : 0xff777777u;
    graph_round(frame, r->x, r->y, r->w, r->h, radius,
            wm.theme.frameW, color);
}

static void draw_title(graph_t *desktop, graph_t *g, xinfo_t *info,
        grect_t *r, bool top, void *data) {
    (void)desktop; (void)data;
    uint32_t bg = top ? wm.theme.frameBGColor : 0xff777777u;
    uint32_t fg = top ? wm.theme.frameFGColor : 0xffddddddU;
    graph_fill_rect(g, r->x, r->y, r->w, r->h, bg);
    if(font != NULL)
        graph_draw_text_font(g, r->x + wm.theme.titleH + 6, r->y + 4,
                info->title, font, wm.theme.fontSize, fg);
}

static void draw_close(graph_t *g, xinfo_t *info, grect_t *r,
        bool top, void *data) {
    (void)info; (void)top; (void)data;
    graph_fill_circle(g, r->x + r->w / 2, r->y + r->h / 2,
            r->w / 2 - 4, 0xffdd6666u);
}

static void draw_max(graph_t *g, xinfo_t *info, grect_t *r,
        bool top, void *data) {
    (void)info; (void)top; (void)data;
    graph_fill_circle(g, r->x + r->w / 2, r->y + r->h / 2,
            r->w / 2 - 4, 0xff66aa22u);
}

static void draw_min(graph_t *g, xinfo_t *info, grect_t *r,
        bool top, void *data) {
    (void)info; (void)top; (void)data;
    graph_fill_circle(g, r->x + r->w / 2, r->y + r->h / 2,
            r->w / 2 - 4, 0xffe0b030u);
}

static void draw_resize(graph_t *g, xinfo_t *info, grect_t *r,
        bool top, void *data) {
    (void)info; (void)data;
    if(top)
        graph_line(g, r->x, r->y + r->h - 1,
                r->x + r->w - 1, r->y, wm.theme.frameFGColor);
}

int ewok_service_init(void) {
    memset(&wm, 0, sizeof(wm));
    wm.theme.desktopBGColor = 0xff555588u;
    wm.theme.desktopFGColor = 0xff8888aau;
    wm.theme.frameBGColor = 0xffaaaaaau;
    wm.theme.frameFGColor = 0xff222222u;
    wm.theme.frameW = 1;
    wm.theme.titleH = 20;
    wm.theme.fontSize = 13;
    wm.theme.round = 13;
    wm.theme.shadow = 0;
    wm.theme.frameAlpha = true;
    strcpy(wm.theme.fontName, "system");
    strcpy(wm.theme.patternName,
            "/usr/system/images/wallpapers/wallpaper1.png");
    reload_theme_assets();
    if(desktop_pattern != NULL)
        klog("xwm.wasm: decoded desktop wallpaper %dx%d\n",
                desktop_pattern->w, desktop_pattern->h);
    else
        klog("xwm.wasm: failed to decode desktop wallpaper\n");
    wm.get_win_space = get_win_space;
    wm.get_title = get_title;
    wm.get_close = get_close;
    wm.get_max = get_max;
    wm.get_min = get_min;
    wm.get_resize = get_resize;
    wm.get_frame = get_frame;
    wm.get_min_size = get_min_size;
    wm.draw_desktop = draw_desktop;
    wm.draw_frame = draw_frame;
    wm.draw_title = draw_title;
    wm.draw_close = draw_close;
    wm.draw_max = draw_max;
    wm.draw_min = draw_min;
    wm.draw_resize = draw_resize;
    wm.update_theme = update_theme;

    /* xserverd starts before the window manager and therefore has no XWM
     * theme yet. Publish the exact theme used to size and paint frames so
     * the compositor blends the rounded alpha ring instead of copying its
     * transparent pixels as opaque black. Do this before registering the
     * XWM, otherwise xserverd synchronously sends the theme back to this
     * still-running Wasm context. */
    proto_t in;
    PF->init(&in)->add(&in, &wm.theme, sizeof(wm.theme));
    int result = dev_cntl("/dev/x", X_DCNTL_SET_XWM_THEME, &in, NULL);
    PF->clear(&in);
    if(result != 0)
        klog("xwm.wasm: failed to publish compositor theme\n");
    return xwm_start(&wm);
}

int ewok_service_step(void) {
    return 0;
}
