//
// Created by Hugo Trippaers on 13/02/2023.
//

#ifndef LEDPANEL_FRAMEBUFFER_H
#define LEDPANEL_FRAMEBUFFER_H

typedef struct {
    int pin_r0, pin_g0, pin_b0;
    int pin_r1, pin_g1, pin_b1;
    int pin_clk;
    int pin_lat;
    int pin_oe;
    int pin_a, pin_b, pin_c;
    int w, h, bpp;
} framebuffer_config_t;

typedef struct {
    void *buffer;
    framebuffer_config_t config;
} framebuffer_t;

#define FRAMEBUFFER_OK 0
#define FRAMEBUFFER_ERROR 1

int framebuffer_init(framebuffer_config_t config, framebuffer_t *framebuffer);
int framebuffer_sync(framebuffer_t *framebuffer);

#endif //LEDPANEL_FRAMEBUFFER_H
