#include <pico/stdio.h>
#include <pico/printf.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <math.h>
#include <hardware/i2c.h>
#include "framebuffer.h"
#include "animations/animations.h"

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

#define I2C_ADDRESS 0x50
#define I2C_1_SCL 15
#define I2C_1_SDA 14

framebuffer_t fb;
framebuffer_config_t framebuffer_config = {
    R0,G0, B0,
    R1, G1, B1,
    CLK, LAT, OE,
    A, B, C,
    DISPLAY_W, DISPLAY_H, DISPLAY_BPP,
    .oe_inverted = false // LOW = off
};

static void core1_entry();
static void i2c1_slave_irq_handler();

bool timer_callback(repeating_timer_t *user_data) {
    gif_animation_update(&fb);
    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("PicoPlayer Starting\n");

    // Enable led on boot
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Enabled I2C
    i2c_init(i2c1, 100 * 1000);

    gpio_init(I2C_1_SCL);
    gpio_set_function(I2C_1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_1_SCL);

    gpio_init(I2C_1_SDA);
    gpio_set_function(I2C_1_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_1_SDA);

    i2c_set_slave_mode(i2c1, true, I2C_ADDRESS);

    i2c_hw_t *hw = i2c_get_hw(i2c1);
    hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_RD_REQ_BITS | I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS | I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_START_DET_BITS;
    irq_set_exclusive_handler(I2C1_IRQ, i2c1_slave_irq_handler);
    irq_set_enabled(I2C1_IRQ, true);

    // Start all the framebuffer work on the second core
    //multicore_launch_core1(core1_entry);

    gif_animation_init(&fb);

    repeating_timer_t timer;
    alarm_pool_t *alarm_pool = alarm_pool_create_with_unused_hardware_alarm(1);
    alarm_pool_add_repeating_timer_us(alarm_pool, 60 * (int64_t)1000, timer_callback, NULL, &timer);

    // Core issues with timers, run on main core for now
    core1_entry();
    while (1) {
        tight_loop_contents();
    }
}

static void core1_entry() {
    if (framebuffer_init(framebuffer_config, &fb) != FRAMEBUFFER_OK) {
        return;
    }

    while (1) {
        framebuffer_sync(&fb);
    }
}

int transfer_in_progress = false;
static void __not_in_flash_func(i2c1_slave_irq_handler)() {
    i2c_hw_t *hw = i2c_get_hw(i2c1);

    uint32_t intr_stat = hw->intr_stat;
    if (intr_stat == 0) {
        return;
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {
        hw->clr_tx_abrt;
        if (transfer_in_progress) {
            transfer_in_progress = false;
        }
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_START_DET_BITS) {
        hw->clr_start_det;
        if (transfer_in_progress) {
            transfer_in_progress = false;
        }
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
        hw->clr_stop_det;
        if (transfer_in_progress) {
            transfer_in_progress = false;
        }
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        transfer_in_progress = true;
        printf("I2C Received: %02x\n", i2c_read_byte_raw(i2c1));
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
        hw->clr_rd_req;
        transfer_in_progress = true;
        i2c_write_byte_raw(i2c1, 0x42);
        printf("I2C Sent: %02x\n", 0x42);
    }
}
