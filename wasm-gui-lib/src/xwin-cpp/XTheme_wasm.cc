#include "x++/XTheme.h"
#include <wasm_ewok_compat.h>
#include <string.h>

using namespace Ewok;

XTheme::XTheme() {
    font = NULL;
    
    // Initialize with default WebAssembly theme
    memset(&basic, 0, sizeof(x_theme_t));
    basic.uuid = 1;
    strcpy(basic.name, "default");
    strcpy(basic.fontName, "Arial");
    basic.fontSize = 14;
    basic.charSpace = 1;
    basic.lineSpace = 2;
    
    basic.bgColor = 0xfff0f0f0;
    basic.fgColor = 0xff000000;
    basic.docBGColor = 0xffffffff;
    basic.docFGColor = 0xff000000;
    basic.bgDisableColor = 0xffc0c0c0;
    basic.fgDisableColor = 0xff808080;
    basic.selectColor = 0xff0078d4;
    basic.selectBGColor = 0xff0078d4;
    basic.titleColor = 0xffffffff;
    basic.titleBGColor = 0xff0078d4;
    
    font = font_new(basic.fontName, false);
}

void XTheme::setFont(const char* name, uint32_t size) {
    if (font) {
        font_free(font);
    }
    
    if (name) {
        strncpy(basic.fontName, name, FONT_NAME_MAX - 1);
        basic.fontName[FONT_NAME_MAX - 1] = '\0';
    }
    
    if (size > 0) {
        basic.fontSize = size;
    }
    
    font = font_new(basic.fontName, false);
}

void XTheme::loadSystem(void) {
    // WebAssembly stub - theme is already loaded
    // In a real implementation, this could load theme from browser storage
}

XTheme* XTheme::dup(XTheme* theme) {
    if (!theme) return NULL;
    
    XTheme* newTheme = new XTheme();
    memcpy(&newTheme->basic, &theme->basic, sizeof(x_theme_t));
    
    if (newTheme->font) {
        font_free(newTheme->font);
    }
    newTheme->font = font_new(theme->basic.fontName, false);
    
    return newTheme;
}