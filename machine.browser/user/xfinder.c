#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ewoksys/keydef.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <graph/graph_image.h>
#include <mouse/mouse.h>
#include <x/x.h>
#include <x/xwin.h>

#define MAX_FILES 96
#define TOOLBAR_HEIGHT 48
#define FOOTER_HEIGHT 30
#define CELL_HEIGHT 88
#define MIN_CELL_WIDTH 92
#define ICON_SIZE 48

typedef struct {
    char name[FS_NODE_NAME_MAX];
    uint8_t type;
} file_item_t;

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static graph_t *folder_icon;
static graph_t *file_icon;
static graph_t *device_icon;
static file_item_t files[MAX_FILES];
static int file_count;
static int page_start;
static int selected = -1;
static char current_path[FS_FULL_NAME_MAX + 1] = "/";
static char status_text[96];

static void draw_text(graph_t *g, int x, int y, const char *text,
        uint32_t color) {
    if(app_font != NULL)
        graph_draw_text_font(g, x, y, text, app_font,
                app_theme.fontSize, color);
}

static void draw_button(graph_t *g, int x, int y, int w, int h,
        const char *label, uint32_t color) {
    graph_fill_round(g, x, y, w, h, 7, color);
    graph_round(g, x, y, w, h, 7, 1, 0x88ffffffu);
    if(app_font != NULL)
        graph_draw_text_font_align(g, x, y + 1, w, h - 2, label,
                app_font, app_theme.fontSize, 0xffffffffu,
                FONT_ALIGN_CENTER);
}

static int item_before(const file_item_t *left, const file_item_t *right) {
    int left_dir = left->type == DT_DIR;
    int right_dir = right->type == DT_DIR;
    if(left_dir != right_dir)
        return left_dir > right_dir;
    return strcmp(left->name, right->name) < 0;
}

static void sort_files(void) {
    for(int i = 1; i < file_count; i++) {
        file_item_t value = files[i];
        int j = i;
        while(j > 0 && item_before(&value, &files[j - 1])) {
            files[j] = files[j - 1];
            j--;
        }
        files[j] = value;
    }
}

static void load_directory(const char *path) {
    char requested[FS_FULL_NAME_MAX + 1];
    memset(requested, 0, sizeof(requested));
    strncpy(requested, path, sizeof(requested) - 1);
    DIR *directory = opendir(requested);
    if(directory == NULL) {
        snprintf(status_text, sizeof(status_text),
                "Cannot open %s", requested);
        return;
    }

    file_count = 0;
    memset(files, 0, sizeof(files));
    struct dirent *entry;
    while(file_count < MAX_FILES && (entry = readdir(directory)) != NULL) {
        if(entry->d_name[0] == '.')
            continue;
        strncpy(files[file_count].name, entry->d_name,
                sizeof(files[file_count].name) - 1);
        files[file_count].type = entry->d_type;
        file_count++;
    }
    closedir(directory);
    sort_files();
    memset(current_path, 0, sizeof(current_path));
    strncpy(current_path, requested, sizeof(current_path) - 1);
    page_start = 0;
    selected = -1;
    snprintf(status_text, sizeof(status_text), "%d items", file_count);
}

static void make_full_path(const char *name, char *path, uint32_t size) {
    if(strcmp(current_path, "/") == 0)
        snprintf(path, size, "/%s", name);
    else
        snprintf(path, size, "%s/%s", current_path, name);
}

static int has_suffix(const char *name, const char *suffix) {
    uint32_t name_size = strlen(name);
    uint32_t suffix_size = strlen(suffix);
    return name_size >= suffix_size &&
            strcmp(name + name_size - suffix_size, suffix) == 0;
}

static void go_up(void) {
    if(strcmp(current_path, "/") == 0)
        return;
    char parent[FS_FULL_NAME_MAX + 1];
    memset(parent, 0, sizeof(parent));
    strncpy(parent, current_path, sizeof(parent) - 1);
    int length = (int)strlen(parent);
    while(length > 0 && parent[length - 1] != '/')
        parent[--length] = 0;
    if(length > 1)
        parent[length - 1] = 0;
    else
        strcpy(parent, "/");
    load_directory(parent);
}

static void open_item(int index) {
    if(index < 0 || index >= file_count)
        return;
    selected = index;
    char path[FS_FULL_NAME_MAX + 1];
    make_full_path(files[index].name, path, sizeof(path));
    if(files[index].type == DT_DIR) {
        load_directory(path);
        return;
    }
    char command[FS_FULL_NAME_MAX + 48];
    if(has_suffix(path, ".png"))
        snprintf(command, sizeof(command), "/apps/ximg/ximg \"%s\"", path);
    else if(has_suffix(path, ".txt") || has_suffix(path, ".conf") ||
            has_suffix(path, ".json") || has_suffix(path, ".rd") ||
            has_suffix(path, ".js") || has_suffix(path, ".xml") ||
            has_suffix(path, ".md") || has_suffix(path, ".log"))
        snprintf(command, sizeof(command), "/apps/xread/xread \"%s\"", path);
    else
        snprintf(command, sizeof(command), "%s", path);
    x_exec(command);
    snprintf(status_text, sizeof(status_text), "Opening %s",
            files[index].name);
}

static void grid_size(int width, int height, int *columns, int *rows,
        int *cell_width) {
    *columns = width / MIN_CELL_WIDTH;
    if(*columns < 3)
        *columns = 3;
    if(*columns > 7)
        *columns = 7;
    *cell_width = width / *columns;
    *rows = (height - TOOLBAR_HEIGHT - FOOTER_HEIGHT) / CELL_HEIGHT;
    if(*rows < 1)
        *rows = 1;
}

static graph_t *item_icon(const file_item_t *item) {
    if(item->type == DT_DIR)
        return folder_icon;
    if(item->type == DT_CHR || item->type == DT_BLK)
        return device_icon;
    return file_icon;
}

static void display_name(const char *name, char *label, uint32_t size) {
    uint32_t length = strlen(name);
    if(length < size) {
        strcpy(label, name);
        return;
    }
    if(size < 5)
        return;
    memcpy(label, name, size - 4);
    strcpy(label + size - 4, "...");
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xff17212bu);
    graph_fill_rect(g, 0, 0, g->w, TOOLBAR_HEIGHT, 0xff263440u);
    draw_button(g, 10, 9, 54, 30, "Up", 0xff315c7au);
    draw_button(g, 72, 9, 76, 30, "Refresh", 0xff315c7au);
    graph_fill_round(g, 158, 9, g->w - 168, 30, 7, 0xff101820u);
    draw_text(g, 170, 17, current_path, 0xffeeeeeeu);

    int columns, rows, cell_width;
    grid_size(g->w, g->h, &columns, &rows, &cell_width);
    int visible = columns * rows;
    for(int slot = 0; slot < visible; slot++) {
        int index = page_start + slot;
        if(index >= file_count)
            break;
        int column = slot % columns;
        int row = slot / columns;
        int x = column * cell_width;
        int y = TOOLBAR_HEIGHT + row * CELL_HEIGHT;
        if(index == selected) {
            graph_fill_round(g, x + 4, y + 3, cell_width - 8,
                    CELL_HEIGHT - 6, 9, 0xff315c7au);
            graph_round(g, x + 4, y + 3, cell_width - 8,
                    CELL_HEIGHT - 6, 9, 1, 0xff75c7ffu);
        }
        graph_t *icon = item_icon(&files[index]);
        if(icon != NULL) {
            int icon_x = x + (cell_width - ICON_SIZE) / 2;
            graph_blt_fit_alpha(icon, 0, 0, icon->w, icon->h, g,
                    icon_x, y + 5, ICON_SIZE, ICON_SIZE, 0xff);
        }
        char label[18] = {0};
        display_name(files[index].name, label, sizeof(label));
        if(app_font != NULL)
            graph_draw_text_font_align(g, x + 2, y + 59,
                    cell_width - 4, 20, label, app_font,
                    app_theme.fontSize, 0xffeeeeeeu, FONT_ALIGN_CENTER);
    }

    int footer_y = g->h - FOOTER_HEIGHT;
    graph_fill_rect(g, 0, footer_y, g->w, FOOTER_HEIGHT, 0xff263440u);
    draw_text(g, 10, footer_y + 8, status_text, 0xffa9c1d1u);
    draw_button(g, g->w - 148, footer_y + 3, 64, 24,
            "Prev", page_start > 0 ? 0xff315c7au : 0xff38434cu);
    draw_button(g, g->w - 76, footer_y + 3, 64, 24,
            "Next", page_start + visible < file_count ?
            0xff315c7au : 0xff38434cu);
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type != XEVT_MOUSE)
        return;

    int columns, rows, cell_width;
    int width = window->xinfo->wsr.w;
    int height = window->xinfo->wsr.h;
    grid_size(width, height, &columns, &rows, &cell_width);
    int visible = columns * rows;
    if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
        page_start -= columns;
        if(page_start < 0)
            page_start = 0;
        xwin_repaint(window);
        return;
    }
    if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
        int last_start = file_count > visible ? file_count - visible : 0;
        page_start += columns;
        if(page_start > last_start)
            page_start = last_start;
        xwin_repaint(window);
        return;
    }
    if(event_value->state != MOUSE_STATE_CLICK)
        return;

    gpos_t pos = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);
    if(pos.y < TOOLBAR_HEIGHT) {
        if(pos.x >= 10 && pos.x < 64)
            go_up();
        else if(pos.x >= 72 && pos.x < 148)
            load_directory(current_path);
        xwin_repaint(window);
        return;
    }

    if(pos.y >= height - FOOTER_HEIGHT) {
        if(pos.x >= width - 148 && pos.x < width - 84 && page_start > 0) {
            page_start -= visible;
            if(page_start < 0)
                page_start = 0;
        }
        else if(pos.x >= width - 76 && pos.x < width - 12 &&
                page_start + visible < file_count)
            page_start += visible;
        xwin_repaint(window);
        return;
    }

    int column = pos.x / cell_width;
    int row = (pos.y - TOOLBAR_HEIGHT) / CELL_HEIGHT;
    if(column >= 0 && column < columns && row >= 0 && row < rows)
        open_item(page_start + row * columns + column);
    xwin_repaint(window);
}

int ewok_service_init(void) {
    memset(&app_x, 0, sizeof(app_x));
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    folder_icon = graph_image_new("/apps/xfinder/res/icons/folder.png");
    file_icon = graph_image_new("/apps/xfinder/res/icons/file.png");
    device_icon = graph_image_new("/apps/xfinder/res/icons/device.png");
    load_directory("/");
    app_window = xwin_open(&app_x, -1, 80, 70, 620, 420,
            "xfinder.wasm", XWIN_STYLE_NORMAL |
            XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL || app_font == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/xfinder/xfinder");
    xwin_set_visible(app_window, true);
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return -1;
    for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    return 0;
}
