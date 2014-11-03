#ifndef COLOR_H_
#define COLOR_H_

#include <stdint.h>

typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} rgb_t;

typedef struct {
    uint16_t h;
    uint16_t s;
    uint16_t v;
} hsv_t;

rgb_t hsv2rgb(hsv_t c);

#endif
