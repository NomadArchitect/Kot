#include <kot-graphics/utils.h>
#include <kot-graphics/context.h>

int8_t pixelExist(framebuffer_t* fb, uint32_t x, uint32_t y) {
    if (x < 0 || y < 0) return -1;
    if (x > fb->Width || y > fb->Height) return -1;
    return 1;
}

void putPixel(framebuffer_t* fb, uint32_t x, uint32_t y, uint32_t colour) {
    if (pixelExist(fb, x, y) == -1) return;
    uint64_t index = x * 4 + y * fb->Pitch;
    blendAlpha(((uint64_t)fb->Buffer + index), colour);
}

uint32_t getPixel(framebuffer_t* fb, uint32_t x, uint32_t y) {
    uint64_t index = x * 4 + y * fb->Pitch;
    return *(uint32_t*)((uint64_t) fb->Buffer + index);
}

void blitFramebuffer(framebuffer_t* to, framebuffer_t* from, uint32_t x, uint32_t y) {

    uint64_t to_addr = (uint64_t) to->Buffer;
    uint64_t from_addr = (uint64_t) from->Buffer;

    to_addr += x * to->Btpp + y * to->Pitch; // offset

    uint64_t num;

    if (to->Pitch < from->Pitch) {
        num = to->Pitch;
    } else {
        num = from->Pitch;
    } 

    for (uint32_t h = 0; h < from->Height && h + y < to->Height; h++) {
        memcpy((uintptr_t) to_addr, (uintptr_t) from_addr, num);
        to_addr += to->Pitch;
        from_addr += from->Pitch;
    }

}

void blitFramebufferRadius(framebuffer_t* to, framebuffer_t* from, uint32_t x, uint32_t y, uint16_t borderRadius) {
    uint64_t to_addr = (uint64_t) to->Buffer;
    uint64_t from_addr = (uint64_t) from->Buffer;

    to_addr += x * to->Btpp + y * to->Pitch; // offset

    uint64_t num;

    if (to->Pitch < from->Pitch) {
        num = to->Pitch;
    } else {
        num = from->Pitch;
    }

    for (uint32_t h = 0; h < from->Height && h + y < to->Height; h++) {
        memcpy((uintptr_t) to_addr, (uintptr_t) from_addr, num);
        to_addr += to->Pitch;
        from_addr += from->Pitch;
    }
}

void fillRect(framebuffer_t* fb, uint32_t x, uint32_t y, uint32_t Width, uint32_t Height, uint32_t colour) {

    uint32_t _h = Height+y;
    uint32_t _w = Width+x;

    if (_h > fb->Height) { _h = fb->Height; }
    if (_w > fb->Width) { _w = fb->Width; }

    for (uint32_t h = y; h < _h; h++) {
        uint64_t YPosition = h * fb->Pitch;
        for (uint32_t w = x; w < _w; w++) {
            uint64_t XPosition = w * fb->Btpp;
            uint64_t index = YPosition + XPosition;
            blendAlpha(((uint64_t)fb->Buffer + index), colour);
        }
    }

}

void drawLine(framebuffer_t* fb, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t colour) {

    if (x1 > fb->Width) { x1 = fb->Width; }
    if (y1 > fb->Height) { y1 = fb->Height; }
    if (x2 > fb->Width) { x2 = fb->Width; }
    if (y2 > fb->Height) { y2 = fb->Height; }

    if (x1 < 0) { x1 = 0; }
    if (y1 < 0) { y1 = 0; }
    if (x2 < 0) { x2 = 0; }
    if (y2 < 0) { y2 = 0; }

    int32_t dx = x2-x1;
    int32_t dy = y2-y1;

    int8_t sx = sgn(dx);
    int8_t sy = sgn(dy);

    int32_t x = x1;
    int32_t y = y1;

    int8_t isSwaped = 0;

    if(abs(dy) > abs(dx)) {
        int32_t tdx = dx;
        dx = dy;
        dy = tdx;
        isSwaped = 1;
    }

    int32_t p = 2*(abs(dy)) - abs(dx);

    putPixel(fb, x, y, colour);

    for (int32_t i = 0; i < abs(dx); i++) {
        if (p < 0) {
            if (isSwaped == 0) {
                x = x + sx;
                putPixel(fb, x, y, colour);
            } else {
                y = y+sy;
                putPixel(fb, x, y, colour);
            }
            p = p + 2*abs(dy);
        } else {
            x = x+sx;
            y = y+sy;
            putPixel(fb, x, y, colour);
            p = p + 2*abs(dy) - 2*abs(dx);
        }
    }

}

void drawRect(framebuffer_t* fb, uint32_t x, uint32_t y, uint32_t Width, uint32_t Height, uint32_t colour) {
    drawLine(fb, x, y, x+Width, y, colour); // top
    drawLine(fb, x, y+Height, x+Width, y+Height, colour); // bottom
    drawLine(fb, x, y, x, y+Height, colour); // left
    drawLine(fb, x+Width, y, x+Width, y+Height, colour); // right
}