//
// Created by Hugo Trippaers on 23/07/2023.
//

#ifndef LEDPANEL_ANIMATIONS_H
#define LEDPANEL_ANIMATIONS_H

#include "framebuffer.h"

void plasma_init(framebuffer_t *framebuffer);
void plasma_update(framebuffer_t *framebuffer);

void gif_animation_init(framebuffer_t *framebuffer);
void gif_animation_update(framebuffer_t *framebuffer);

#endif //LEDPANEL_ANIMATIONS_H
