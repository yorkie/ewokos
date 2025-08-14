#ifndef FONT_H 
#define FONT_H

#include <stdint.h>
#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SYSTEM_FONT       "Arial"
#define DEFAULT_SYSTEM_FONT_FILE  "Arial"
#define DEFAULT_SYSTEM_FONT_SIZE  12

#define FONT_ALIGN_NONE    0x00
#define FONT_ALIGN_CENTER  0x01

#define FONT_NAME_MAX 64

typedef struct {
    int16_t  ascender;
    int16_t  descender;
    uint32_t height;
    uint32_t width;
} face_info_t;

typedef struct {
    char name[FONT_NAME_MAX];
    int32_t id;
} font_t;

int     font_init(void);

font_t* font_new(const char* fname, bool fixed);
int     font_load(const char* name, const char* fname);
void    font_free(font_t* font);

gsize_t font_text_size(font_t* font, const char* str, uint32_t len, int size);
const char* font_get_name(font_t* font);
bool    font_fixed(font_t* font);
const char* font_name_by_fname(const char* fname);

#ifdef __cplusplus
}
#endif

#endif