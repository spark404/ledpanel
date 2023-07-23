#include <pico/stdio.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <pico/printf.h>
#include "pico/multicore.h"
#include <math.h>
#include <malloc.h>
#include "framebuffer.h"
#include "rgb565.h"
#include "gif_decoder.h"

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

extern uint8_t seasons_gif_start[] asm( "_binary_seasons_gif_start" );
extern uint8_t seasons_gif_end[]   asm( "_binary_seasons_gif_end" );
extern size_t seasons_gif_size  asm( "_binary_seasons_gif_size" );

framebuffer_t _fb;
framebuffer_config_t framebuffer_config = {
    R0,G0, B0,
    R1, G1, B1,
    CLK, LAT, OE,
    A, B, C,
    DISPLAY_W, DISPLAY_H, DISPLAY_BPP,
    .oe_inverted = false // LOW = off
};

gif_t gif;

void core1_entry();
void plasma_init(framebuffer_t *framebuffer);
void plasma_update(framebuffer_t *framebuffer);

bool timer_callback(repeating_timer_t *user_data) {
    // plasma_update(&_fb);
    frame_t frame;
    gif_decoder_read_image(&gif, &frame);

    uint8_t *frame_ptr = frame.frame;
    for (int y=0; y<frame.height; y++) {
        for (int x=0; x<frame.width; y++) {
            uint32_t c = frame_ptr[0]<<16 | frame_ptr[1]<<8 | frame_ptr[2];
            framebuffer_drawpixel(&_fb, x+frame.offset_x, y+frame.offset_y, c);
        }
    }
    free(frame.frame);

    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    if (framebuffer_init(framebuffer_config, &_fb) != FRAMEBUFFER_OK) {
        return -1;
    }

    // Enable led on boot
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    //plasma_init(&_fb);
    gif_decoder_init(seasons_gif_start, seasons_gif_end - seasons_gif_start, &gif);

    repeating_timer_t timer;
    add_repeating_timer_ms(5000, &timer_callback, NULL, &timer);

    multicore_launch_core1(core1_entry);
    while (1) {
        tight_loop_contents();
    }
}

void core1_entry() {
    while (1) {
        framebuffer_sync(&_fb);
    }
}

double cos_table[256];
uint8_t colour_map[256][3];
uint8_t ptn_table[4];
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