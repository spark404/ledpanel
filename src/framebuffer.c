//
// Created by Hugo Trippaers on 13/02/2023.
//


#include <malloc.h>
#include <framebuffer.h>
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <string.h>

static void latch(framebuffer_t *framebuffer, int line, int delay);

static inline uint32_t gamma_correct_565_888(uint16_t pix) {
    uint32_t r_gamma = pix & 0xf800u;
    r_gamma *= r_gamma;
    uint32_t g_gamma = pix & 0x07e0u;
    g_gamma *= g_gamma;
    uint32_t b_gamma = pix & 0x001fu;
    b_gamma *= b_gamma;
    return (b_gamma >> 2 << 16) | (g_gamma >> 14 << 8) | (r_gamma >> 24 << 0);
}

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

    framebuffer->buffer = fb;
    framebuffer->config = config;
    framebuffer->pwm = 0;
    return FRAMEBUFFER_OK;
}

// http://www.batsocks.co.uk/readme/art_bcm_5.htm
int framebuffer_sync(framebuffer_t *framebuffer) {
    if (framebuffer->pwm > 7) {
        framebuffer->pwm = 0;
    }

    uint8_t *ptr = framebuffer->buffer;

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

            // Set the y values for the panel top half
            gpio_put(framebuffer->config.pin_r0, t_r);
            gpio_put(framebuffer->config.pin_g0, t_g);
            gpio_put(framebuffer->config.pin_b0, t_b);

            // Set the y values for the panel bottom half
            gpio_put(framebuffer->config.pin_r1, b_r);
            gpio_put(framebuffer->config.pin_g1, b_g);
            gpio_put(framebuffer->config.pin_b1, b_b);

            // Shift the register into the shifter
            gpio_put(framebuffer->config.pin_clk, 1);
            busy_wait_us(1); // TODO this could use some tweaking
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
    busy_wait_us(1);
    gpio_put(framebuffer->config.pin_lat, 0);

    // Set output enable HIGH to turn on the display
    gpio_put(framebuffer->config.pin_oe, 1);

    busy_wait_us(delay);
}
