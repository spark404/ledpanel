//
// Created by Hugo Trippaers on 23/07/2023.
//

#include <malloc.h>
#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"
#include "gif_decoder.h"

#include "framebuffer.h"

extern uint8_t seasons_gif_start[] asm( "_binary_images_seasons_gif_start" );
extern uint8_t seasons_gif_end[]   asm( "_binary_images_seasons_gif_end" );
extern uint8_t baloons_gif_start[] asm( "images_baloons_gif_start" );
extern uint8_t baloons_gif_end[]   asm( "images_baloons_gif_end" );
extern uint8_t fishandcat_gif_start[] asm( "images_fishandcat_gif_start" );
extern uint8_t fishandcat_gif_end[]   asm( "images_fishandcat_gif_end" );
extern uint8_t skull_gif_start[] asm( "images_skull_gif_start" );
extern uint8_t skull_gif_end[]   asm( "images_skull_gif_end" );

static gif_t gif;

void gif_animation_init(framebuffer_t *framebuffer) {
    gif_decoder_init(fishandcat_gif_start, fishandcat_gif_end - fishandcat_gif_start, &gif);
}

void gif_animation_update(framebuffer_t *framebuffer) {
    frame_t frame;
    if (gif_decoder_read_image(&gif, &frame) != GIF_OK) {
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