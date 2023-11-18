//
// Created by Hugo Trippaers on 23/07/2023.
//

#ifndef LEDPANEL_ANIMATIONS_H
#define LEDPANEL_ANIMATIONS_H

#include "framebuffer.h"

#define DEFAULT_GIF_SEQUENCE 11

void plasma_init(framebuffer_t *framebuffer);
void plasma_update(framebuffer_t *framebuffer);

void gif_animation_init(framebuffer_t *framebuffer);
void gif_animation_update(framebuffer_t *framebuffer);
void gif_animation_play(int sequence_id, int state);
void gif_animation_pause();
void gif_animation_resume();
void gif_animation_stop();
uint8_t gif_animation_get_state();
uint8_t gif_animation_get_sequence();

#endif //LEDPANEL_ANIMATIONS_H
