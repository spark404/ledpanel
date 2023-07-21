#include <pico/stdio.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <pico/printf.h>
#include <math.h>
#include "framebuffer.h"
#include "rgb565.h"

#define B0 7
#define G0 8
#define R0 9
#define B1 11
#define G1 12
#define R1 13
#define CLK 16
#define C 18
#define B 19
#define A 20
#define OE 21
#define LAT 22

#define DISPLAY_W 32
#define DISPLAY_H 16
#define DISPLAY_BPP 32

framebuffer_t _fb;
framebuffer_config_t framebuffer_config = {
    R0,G0, B0,
    R1, G1, B1,
    CLK, LAT, OE,
    A, B, C,
    DISPLAY_W, DISPLAY_H, DISPLAY_BPP,
    .oe_inverted = false // LOW = off
};

// Rainbow
void rainbow_init(framebuffer_t *framebuffer);
void bright_init(framebuffer_t *framebuffer);
void bright_test_init(framebuffer_t *framebuffer);
void bright_test_update(framebuffer_t *framebuffer);
void plasma_screen(framebuffer_t *framebuffer);

bool timer_callback(repeating_timer_t *user_data) {
    bright_test_update(&_fb);
    // plasma_screen(&_fb);
    return true;
}

int main(void) {
    stdio_init_all();

    if (framebuffer_init(framebuffer_config, &_fb) != FRAMEBUFFER_OK) {
        return -1;
    }

    // Enable the boot
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

//    rainbow_init(&_fb);
    bright_test_init(&_fb);
    repeating_timer_t timer;
    add_repeating_timer_ms(250, &timer_callback, NULL, &timer);

    while (1) {
        framebuffer_sync(&_fb);
    }
}

void rainbow_init(framebuffer_t *framebuffer) {
    for (int i=0 ; i<32; i++) {
        framebuffer_drawpixel(framebuffer, i, 0, White);
        framebuffer_drawpixel(framebuffer, i, 1, Red);
        framebuffer_drawpixel(framebuffer, i, 2, Green);
        framebuffer_drawpixel(framebuffer, i, 3, Blue);
        framebuffer_drawpixel(framebuffer, i, 4, White);
        framebuffer_drawpixel(framebuffer, i, 5, Cyan);
        framebuffer_drawpixel(framebuffer, i, 6, Magenta);
        framebuffer_drawpixel(framebuffer, i, 7, Yellow);
        framebuffer_drawpixel(framebuffer, i, 8, Olive);
        framebuffer_drawpixel(framebuffer, i, 9, Purple);
        framebuffer_drawpixel(framebuffer, i, 10, DarkCyan);
        framebuffer_drawpixel(framebuffer, i, 11, DarkGrey);
        framebuffer_drawpixel(framebuffer, i, 12, Navy);
        framebuffer_drawpixel(framebuffer, i, 13, DarkGreen);
        framebuffer_drawpixel(framebuffer, i, 14, Maroon);
        framebuffer_drawpixel(framebuffer, i, 15, DarkGrey);
    }
}

void bright_init(framebuffer_t *framebuffer) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t color = y * 16;
            uint16_t rgb565 = ((color >> 3) << 11) |
                    ((color >> 2) << 5) |
                    (color >> 3);
            framebuffer_drawpixel(framebuffer, x, y, rgb565);
        }
    }
}

void bright_test_init(framebuffer_t *framebuffer) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t color = 8;
            if (y > 3) {
                color = 255;
            }
            uint16_t rgb565 = 0x0;
            if (x < 8) {
                rgb565 = ((color >> 3) << 11) | ((color >> 2) << 5) | (color >> 3);
            }
            else if (x < 16) {
                rgb565 = ((color >> 3) << 11);
            }
            else if (x < 24) {
                rgb565 = ((color >> 2) << 5);
            }
            else {
                rgb565 = (color >> 3);
            }

            framebuffer_drawpixel(framebuffer, x, y, rgb565);
            framebuffer_drawpixel(framebuffer, x, y+8, rgb565);
        }
    }
}

int y_step = 0;
int x_step = 0;
int c_step = 0;
void bright_test_update(framebuffer_t *framebuffer) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 32; x++) {
            uint32_t color = (y * 32) << 8;
            int y_pos = (y + y_step) % 8;

            int x_pos = (x + x_step) % 32;
            color |= x_pos * 8;

            color |= c_step << 16;
            framebuffer_drawpixel(framebuffer, x, y_pos, color);
            framebuffer_drawpixel(framebuffer, x, 15-y_pos, color);
        }
    }
    y_step = (y_step + 1) % 8;
    x_step = (x_step + 1) % 32;
    c_step = (c_step + 1) % 256;
}


uint16_t pallette[16] = {
    Blue,
    Navy,
    Navy,
    Navy,

    Navy,
    Navy,
    Navy,
    Navy,

    Blue,
    Navy,
    SkyBlue,
    SkyBlue,

    LightBlue,
    White,
    LightBlue,
    SkyBlue
};

#define SIN8(X) ((sin(X) * 128.0) + 128)
#define COS8(X) ((cos(X) * 128.0) + 128)
#define SIN16(X) (sin(X) * 32767.0)
#define COS16(X) (cos(X) * 32767.0)

int time_counter = 0;
void plasma_screen(framebuffer_t *framebuffer) {
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y <  16; y++) {
            int16_t v = 0;
            uint8_t wibble = SIN8(x);
            v += SIN16(x * wibble * 3 + time_counter);
            v += COS16(y * (128 - wibble)  + time_counter);
            v += SIN16(y * x * COS8(-time_counter)/ 8);

            int palette_idx = ((v >> 8) + 127) >> 4;
            framebuffer_drawpixel(framebuffer, x, y, pallette[palette_idx]);
        }
    }
    time_counter++;
}