#include <kot/heap.h>

#include <kot-ui/font.h>
#include <kot-ui/font/ssfn.h>

kfont_t* LoadFont(uintptr_t data){
    kfont_t* font = malloc(sizeof(kfont_t));
    font->ctx = malloc(sizeof(ssfn_t));

    ssfn_load(font->ctx, data);
    ssfn_select(font->ctx, SSFN_FAMILY_SERIF, NULL, SSFN_STYLE_REGULAR | SSFN_STYLE_UNDERLINE, 64);
    
    return font;
}

void FreeFont(kfont_t* font){
    ssfn_free(font->ctx);
    free(font->ctx);
}

void PrintFont(kfont_t* font, char* str, font_fb_t* buffer, uint64_t x, uint64_t y, uint32_t color){
    ssfn_buf_t ssfnBuff;

    ssfnBuff.ptr = buffer->address;
    ssfnBuff.w = buffer->width;
    ssfnBuff.h = buffer->height;
    ssfnBuff.p = buffer->pitch;
    ssfnBuff.x = x;
    ssfnBuff.y = y;
    ssfnBuff.fg = color;
    ssfnBuff.bg = 0x0;

    ssfn_render(font->ctx, &ssfnBuff, str);
}