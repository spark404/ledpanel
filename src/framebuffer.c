//
// Created by Hugo Trippaers on 13/02/2023.
//


#include <malloc.h>
#include <framebuffer.h>
#include <hardware/gpio.h>
#include <string.h>

static void latch(framebuffer_t *framebuffer, int line, int delay);

int framebuffer_init(framebuffer_config_t config, framebuffer_t *framebuffer) {
    uint pins_mask = 1 << config.pin_r0 |
            1 << config.pin_g0 |
            1 << config.pin_b0 |
            1 << config.pin_r1 |
            1 << config.pin_g1 |
            1 << config.pin_b1 |
            1 << config.pin_clk |
            1 << config.pin_lat |
            1 << config.pin_oe |
            1 << config.pin_a |
            1 << config.pin_b |
            1 << config.pin_c;

    gpio_init_mask(pins_mask);
    gpio_set_dir_out_masked(pins_mask);
    gpio_clr_mask(pins_mask);

    gpio_set_pulls(framebuffer->config.pin_r0, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_g0, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_b0, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_r1, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_g1, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_b1, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_clk, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_lat, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_oe, 0, 1);

    size_t buffer_size = config.w * config.h * (config.bpp / 8);
    void *fb = malloc(buffer_size);
    if (fb == NULL) {
        return FRAMEBUFFER_ERROR;
    }
    bzero(fb, buffer_size);

    framebuffer->buffer_size = buffer_size;
    framebuffer->buffer = fb;
    framebuffer->config = config;
    framebuffer->pwm = 0;
    return FRAMEBUFFER_OK;
}

int framebuffer_clear(framebuffer_t *framebuffer) {
    bzero(framebuffer->buffer, framebuffer->buffer_size);

    return FRAMEBUFFER_OK;
}

// http://www.batsocks.co.uk/readme/art_bcm_5.htm
int framebuffer_sync(framebuffer_t *framebuffer) {
    if (framebuffer->pwm > 7) {
        framebuffer->pwm = 0;
    }

    uint8_t *ptr = framebuffer->buffer;
    uint32_t clr_mask = 1ul << framebuffer->config.pin_r0 |
            1ul << framebuffer->config.pin_g0 |
            1ul << framebuffer->config.pin_b0 |
            1ul << framebuffer->config.pin_r1 |
            1ul << framebuffer->config.pin_g1 |
            1ul << framebuffer->config.pin_b1;

    for (int y = 0; y < framebuffer->config.h / 2 ; y++) {
        int y_idx = y * framebuffer->config.w * 4;
        int y_b_idx = y_idx + (framebuffer->config.w * 4 * 8);
        for (int x = 0; x < framebuffer->config.w; x++) {
            int x_idx = x * 4;


            int t_r = ptr[y_idx + x_idx + 1] >> framebuffer->pwm & 0x1;
            int t_g = ptr[y_idx + x_idx + 2] >> framebuffer->pwm & 0x1;
            int t_b = ptr[y_idx + x_idx + 3] >> framebuffer->pwm & 0x1;

            int b_r = ptr[y_b_idx + x_idx + 1] >> framebuffer->pwm & 0x1;
            int b_g = ptr[y_b_idx + x_idx + 2] >> framebuffer->pwm & 0x1;
            int b_b = ptr[y_b_idx + x_idx + 3] >> framebuffer->pwm & 0x1;

            uint32_t set_mask = t_r << framebuffer->config.pin_r0 |
                                t_g << framebuffer->config.pin_g0 |
                                t_b << framebuffer->config.pin_b0 |
                                b_r << framebuffer->config.pin_r1 |
                                b_g << framebuffer->config.pin_g1 |
                                b_b << framebuffer->config.pin_b1;

            gpio_clr_mask(clr_mask);
            gpio_set_mask(set_mask);
            asm volatile("nop \n nop");

            // Shift the register into the shifter
            gpio_put(framebuffer->config.pin_clk, 1);
            asm volatile("nop \n nop \n nop");

            gpio_put(framebuffer->config.pin_clk, 0);
        }

        // Trigger the latch
        latch(framebuffer, y, 1 << (framebuffer->pwm + 1));
    }

    // Increase pwm cycle (colordepth steps)
    framebuffer->pwm++;

    return FRAMEBUFFER_OK;
}

int framebuffer_drawpixel(framebuffer_t *framebuffer, int x, int y, uint32_t color) {
    if (x < 0 || x >= framebuffer->config.w) {
        return FRAMEBUFFER_ERROR;
    }

    if (y < 0 || y >= framebuffer->config.h) {
        return FRAMEBUFFER_ERROR;
    }

    uint8_t *ptr = framebuffer->buffer;
    int y_idx = y * framebuffer->config.w * 4;
    int x_idx = x * 4;

    ptr[y_idx + x_idx]  = color >> 24;
    ptr[y_idx + x_idx + 1] = color >> 16 & 0xff;
    ptr[y_idx + x_idx + 2]  = color >> 8 & 0xff;
    ptr[y_idx + x_idx + 3] = color & 0xff;

    return FRAMEBUFFER_OK;
}

static void latch(framebuffer_t *framebuffer, int line, int delay) {
    // Select line to latch
    gpio_put(framebuffer->config.pin_a, line & 0x1);
    gpio_put(framebuffer->config.pin_b, (line & 0x2) >> 1);
    gpio_put(framebuffer->config.pin_c, (line & 0x4) >> 2);

    // Set output enable LOW to turn off the display
    gpio_put(framebuffer->config.pin_oe, 0);

    // Trigger the latch by pulsing LAT to HIGH
    gpio_put(framebuffer->config.pin_lat, 1);
    asm volatile("nop \n nop \n nop");
    gpio_put(framebuffer->config.pin_lat, 0);

    // Set output enable HIGH to turn on the display
    gpio_put(framebuffer->config.pin_oe, 1);

    busy_wait_us(delay);
}