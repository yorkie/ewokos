#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <font/font.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    font_t font;
    face_info_t face;
    FT_GlyphSlotRec glyph;

    memset(&font, 0, sizeof(font));
    if(font_init() != 0)
        return -1;
    font.id = font_load(DEFAULT_SYSTEM_FONT, "");
    if(font.id < 0 || font_get_face(&font, 18, &face) != 0)
        return -1;
    if(font_get_glyph_info(&font, 18, 'E', &glyph) != 0 ||
            glyph.bitmap.width == 0 || glyph.bitmap.rows == 0 ||
            glyph.bitmap.buffer == NULL)
        return -1;

    printf("font.wasm: FreeType glyph E %ux%u from ext3 passed\n",
        (unsigned)glyph.bitmap.width, (unsigned)glyph.bitmap.rows);
    free(glyph.bitmap.buffer);
    free(glyph.other);
    return 0;
}
