//
// Created by Hugo Trippaers on 23/07/2023.
//

#include "stdint.h"
#include "math.h"
#include "framebuffer.h"

static double cos_table[256];
static uint8_t colour_map[256][3];
static uint8_t ptn_table[4];

void plasma_init(framebuffer_t *framebuffer) {
    for (int i = 0; i< 256; i++) {
        cos_table[i]= (60 * (cos(i*M_PI/32))) + 4;
    }

    for (int i=0; i<64; i++) {
        colour_map[i][0] = 255;
        colour_map[i][1] = i * 4;
        colour_map[i][2] = 255 - (i * 4);

        colour_map[i+64][0] = 255 - (i * 4);
        colour_map[i+64][1] = 255;
        colour_map[i+64][2] = (i * 4);

        colour_map[i+128][0] = 0;
        colour_map[i+128][1] = 255 - (i * 4);
        colour_map[i+128][2] = 255;

        colour_map[i+192][0] = i * 4;
        colour_map[i+192][1] = 0;
        colour_map[i+192][2] = 255;
    }
}

void plasma_update(framebuffer_t *framebuffer) {
    uint8_t t1 = ptn_table[0];
    uint8_t t2 = ptn_table[1];
    for (int y = 0; y < framebuffer->config.h; y++) {
        uint8_t t3 = ptn_table[2];
        uint8_t t4 = ptn_table[3];
        for (int x = 0; x < framebuffer->config.w; x++) {
            uint32_t colour = cos_table[t1] + cos_table[t2] + cos_table[t3] + cos_table[t4];
            uint32_t c = colour_map[colour][0]<<16|colour_map[colour][1]<<8|colour_map[colour][2];
            framebuffer_drawpixel(framebuffer, x, y, c);
            t3 += 5;
            t4 += 2;
        }
        t1 += 3;
        t2 += 1;
    }

    ptn_table[0] += 1;
    ptn_table[1] += 2;
    ptn_table[2] += 3;
    ptn_table[3] += 4;
}