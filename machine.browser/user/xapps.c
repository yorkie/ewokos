#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <font/font.h>
#include <graph/graph_ex.h>
#include <graph/graph_image.h>
#include <mouse/mouse.h>
#include <x/x.h>
#include <x/xwin.h>

#define MAX_APPS 32
#define GRID_COLUMNS 5
#define GRID_ROWS 4
#define CELL_WIDTH 96
#define CELL_HEIGHT 92
#define ICON_SIZE 48
#define GRID_MARGIN 12

typedef struct {
    char name[32];
    char program[96];
    char icon_path[112];
    graph_t *icon;
} app_item_t;

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static app_item_t apps[MAX_APPS];
static int app_count;
static int selected = -1;

static void load_apps(void) {
    DIR *directory = opendir("/apps");
    if(directory == NULL)
        return;
    struct dirent *entry;
    while(app_count < MAX_APPS && (entry = readdir(directory)) != NULL) {
        if(entry->d_name[0] == '.')
            continue;
        app_item_t *item = &apps[app_count];
        strncpy(item->name, entry->d_name, sizeof(item->name) - 1);
        snprintf(item->program, sizeof(item->program), "/apps/%s/%s",
                entry->d_name, entry->d_name);
        snprintf(item->icon_path, sizeof(item->icon_path),
                "/apps/%s/res/icon.png", entry->d_name);
        item->icon = graph_image_new(item->icon_path);
        app_count++;
    }
    closedir(directory);
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_fill_rect(g, 0, 0, g->w, g->h, 0xff202b35);
    int visible = GRID_COLUMNS * GRID_ROWS;
    if(visible > app_count)
        visible = app_count;
    for(int i = 0; i < visible; i++) {
        int column = i % GRID_COLUMNS;
        int row = i / GRID_COLUMNS;
        int x = GRID_MARGIN + column * CELL_WIDTH;
        int y = GRID_MARGIN + row * CELL_HEIGHT;
        if(i == selected) {
            graph_fill_round(g, x + 3, y + 2, CELL_WIDTH - 6,
                    CELL_HEIGHT - 4, 10, 0xff315c7a);
            graph_round(g, x + 3, y + 2, CELL_WIDTH - 6,
                    CELL_HEIGHT - 4, 10, 2, 0xff75c7ff);
        }
        if(apps[i].icon != NULL) {
            int icon_x = x + (CELL_WIDTH - ICON_SIZE) / 2;
            graph_blt_fit_alpha(apps[i].icon, 0, 0,
                    apps[i].icon->w, apps[i].icon->h, g,
                    icon_x, y + 8, ICON_SIZE, ICON_SIZE, 0xff);
        }
        graph_draw_text_font_align(g, x, y + 62, CELL_WIDTH, 18,
                apps[i].name, app_font, app_theme.fontSize,
                0xffeeeeee, FONT_ALIGN_CENTER);
    }
    if(app_count == 0)
        graph_draw_text_font_align(g, 0, g->h / 2 - 8, g->w, 20,
                "No applications found", app_font, app_theme.fontSize,
                0xffaaaaaa, FONT_ALIGN_CENTER);
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type != XEVT_MOUSE ||
            event_value->state != MOUSE_STATE_CLICK)
        return;
    gpos_t local = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);
    int column = (local.x - GRID_MARGIN) / CELL_WIDTH;
    int row = (local.y - GRID_MARGIN) / CELL_HEIGHT;
    if(local.x < GRID_MARGIN || local.y < GRID_MARGIN ||
            column < 0 || column >= GRID_COLUMNS ||
            row < 0 || row >= GRID_ROWS)
        return;
    int item = row * GRID_COLUMNS + column;
    if(item < 0 || item >= app_count)
        return;
    selected = item;
    xwin_repaint(window);
    x_exec(apps[item].program);
}

int ewok_service_init(void) {
    memset(&app_x, 0, sizeof(app_x));
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    load_apps();
    app_window = xwin_open(&app_x, -1, 350, 110,
            GRID_COLUMNS * CELL_WIDTH + GRID_MARGIN * 2,
            GRID_ROWS * CELL_HEIGHT + GRID_MARGIN * 2,
            "xapps.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL || app_font == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/xapps/xapps");
    xwin_set_visible(app_window, true);
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return -1;
    for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    return 0;
}
