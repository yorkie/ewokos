#ifndef WEB_FONT_H
#define WEB_FONT_H

#include "wasm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Font functions
int     font_init(void);
font_t* font_new(const char* fname, bool safe);
int     font_load(const char* name, const char* fname);
int     font_free(font_t* font);

// Text measurement
void    font_text_size(const char* text, font_t* font, int size, uint32_t* w, uint32_t* h);

#ifdef __cplusplus
}
#endif

#endif // WEB_FONT_H