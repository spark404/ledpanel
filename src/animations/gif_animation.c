//
// Created by Hugo Trippaers on 23/07/2023.
//

#include <malloc.h>
#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"
#include "gif_decoder.h"

#include "framebuffer.h"

extern uint8_t baloons_gif_start[] asm( "images_baloons_gif_start" );
extern uint8_t baloons_gif_end[]   asm( "images_baloons_gif_end" );
extern uint8_t chevrons_gif_start[] asm( "images_chevrons_gif_start" );
extern uint8_t chevrons_gif_end[]   asm( "images_chevrons_gif_end" );
extern uint8_t fishandcat_gif_start[] asm( "images_fishandcat_gif_start" );
extern uint8_t fishandcat_gif_end[]   asm( "images_fishandcat_gif_end" );
extern uint8_t pattern_gif_start[] asm( "images_pattern_gif_start" );
extern uint8_t pattern_gif_end[]   asm( "images_pattern_gif_end" );
extern uint8_t seasons_gif_start[] asm( "images_seasons_gif_start" );
extern uint8_t seasons_gif_end[]   asm( "images_seasons_gif_end" );
extern uint8_t skull_gif_start[] asm( "images_skull_gif_start" );
extern uint8_t skull_gif_end[]   asm( "images_skull_gif_end" );
extern uint8_t snafu_gif_start[] asm( "images_snafu_gif_start" );
extern uint8_t snafu_gif_end[]   asm( "images_snafu_gif_end" );

// Stuck?
extern uint8_t lattice_gif_start[] asm("images_lattice_gif_start");
extern uint8_t lattice_gif_end[] asm("images_lattice_gif_end");

// Interlaces images
extern uint8_t numbers_gif_start[] asm( "images_numbers_gif_start" );
extern uint8_t numbers_gif_end[]   asm( "images_numbers_gif_end" );
extern uint8_t snowing_gif_start[] asm( "images_snowing_gif_start" );
extern uint8_t snowing_gif_end[]   asm( "images_snowing_gif_end" );

static gif_t gif;

void gif_animation_init(framebuffer_t *framebuffer) {
    gif_decoder_init(seasons_gif_start, seasons_gif_end - seasons_gif_start, &gif);
}

void gif_animation_update(framebuffer_t *framebuffer) {
    frame_t frame;
    if (gif_decoder_read_next_frame(&gif, &frame) != GIF_OK) {
        printf("Error in decoder\n");
        return;
    }

    uint8_t *frame_ptr = frame.frame;
    for (int y=0; y<frame.height; y++) {
        for (int x=0; x<frame.width; x++) {
            uint32_t c = frame_ptr[0]<<16 | frame_ptr[1]<<8 | frame_ptr[2];
            frame_ptr+=3;
            if (frame.transparancy_enabled && c == frame.transparent_colour) {
                    continue;
            }
            framebuffer_drawpixel(framebuffer, x+frame.offset_x, y+frame.offset_y, c);
        }
    }
    free(frame.frame);
    frame.frame = NULL;
}