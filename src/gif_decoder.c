//
// Created by Hugo Trippaers on 22/07/2023.
//


#include <string.h>
#include <pico/printf.h>
#include <malloc.h>
#include "gif_decoder.h"
#include "gif_lzw_decompress.h"

#define DEBUG 1

#if DEBUG
#define LOG_MSG printf
#else
#define LOG_MSG(...)
#endif

/*
 * GIF Header
 * Magic (3 bytes)   : GIF
 * Version (3 bytes) : 89a
 * FDSZ (1 byte)
 * Background Index (1)
 * Aspect (1)
 */

gif_error_t gif_decoder_init(uint8_t *source, size_t size, gif_t *gif) {
    uint8_t *ptr = source;
    uint8_t *end_ptr = source + size;

    // Verify magic
    if (memcmp(ptr, "GIF", 3) != 0) {
        LOG_MSG("Invalid magic\n");
        return GIF_ERROR;
    }

    // Verify version
    if (memcmp(ptr+3, "89a", 3) != 0) {
        LOG_MSG("Invalid version\n");
        return GIF_ERROR;
    }

    ptr+=6;

    uint16_t width = *(ptr) | *(ptr+1) << 8;
    uint16_t height = *(ptr+2) | *(ptr+3) << 8;
    uint8_t fdsz = *(ptr+4);
    uint8_t background_idx = *(ptr+5);
    uint8_t aspect = *(ptr+6);

    if (!(fdsz & 0x80)) {
        LOG_MSG("no global color table\n");
        return GIF_ERROR;
    }

    uint8_t depth = ((fdsz >> 4) & 7) + 1;
    uint8_t gct_sz = 1 << ((fdsz & 0x07) + 1);

    LOG_MSG("Got a valid gif %dx%d, depth %d, gct_sz %d, size %d\n", width, height, depth, gct_sz, size);

    gif->image_start = source;
    gif->image_size = size;
    gif->ct_size = gct_sz;
    gif->global_ct = ptr + 7;

    ptr += 7 + (gct_sz * 3);

    int skip_subblock = 0;
    while (ptr < end_ptr && *ptr != BLOCK_IMAGE_DESCRIPTOR) {
        if (skip_subblock) {
            uint8_t block_size = *ptr;
            LOG_MSG("Subblock, size %d\n", *ptr);
            if (!block_size) {
                skip_subblock = 0;
                ptr++;
                continue;
            }
            ptr += block_size + 1;
            continue;
        }

        switch (*ptr) {
            case BLOCK_EXTENSION_INTRODUCER:
                LOG_MSG("Extension block, type 0x%02x, size %d\n", *(ptr+1), *(ptr+2));
                if (*(ptr+1) == 0xFF) {
                    LOG_MSG("Expect subblocks\n");
                    skip_subblock = 1;
                }

                // Skip the extension block
                ptr+= 3 + *(ptr+2);

                // Skip block terminator
                if (!skip_subblock) {
                    ptr++;
                }
                break;
            default:
                LOG_MSG("Not currently pointing to an image descriptor or a known block (0x%02x)\n", *ptr);
                return GIF_ERROR;
        }
    }

    if (ptr>= end_ptr || *ptr != BLOCK_IMAGE_DESCRIPTOR) {
        LOG_MSG("No image descriptor found (0x%02x)\n", *ptr);
        return GIF_ERROR;
    }

    gif->first_frame = ptr;
    gif->frame_ptr = ptr;
    return GIF_OK;
}

gif_error_t gif_decoder_read_image(gif_t *gif, frame_t *frame) {
    uint8_t *end_ptr = gif->image_start + gif->image_size;

    // Read the next available frame
    uint8_t *ptr = gif->frame_ptr;

    if (*ptr != BLOCK_IMAGE_DESCRIPTOR) {
        return GIF_ERROR;
    }
    ptr++;

    uint16_t x_offset = *(ptr) | *(ptr+1) << 8;
    uint16_t y_offset = *(ptr+2) | *(ptr+3) << 8;
    uint16_t width = *(ptr+4) | *(ptr+5) << 8;
    uint16_t height = *(ptr+6) | *(ptr+7) << 8;
    ptr += 8;

    LOG_MSG("New frame with size %dx%d at %d,%d\n", width, height, x_offset, y_offset);
    uint8_t fisrz = *(ptr);

    if (fisrz & 0x40) {
        LOG_MSG("Can't work with interlaced frame\n");
        return GIF_ERROR;
    }

    if (fisrz & 0x80) {
//        gif->lct.size = 1 << ((fisrz & 0x07) + 1);
//        read(gif->fd, gif->lct.colors, 3 * gif->lct.size);
//        gif->palette = &gif->lct;
        LOG_MSG("Can't work with local color table\n");
        return GIF_ERROR;
    }

    frame->offset_x = x_offset;
    frame->offset_y = y_offset;
    frame->width = width;
    frame->height = height;
    frame->frame = malloc(width * height * 3); // Allocate space for all pixels in RGB888 format

    if (frame->frame == NULL) {
        LOG_MSG("Not enough free memory for the frame\n");
        return GIF_ERROR;
    }
    uint8_t buffer[32*16]; // FIXME Good enough for now
    gif_error_t res = gif_decoder_read_image_data(++ptr, buffer);
    if (res != GIF_OK) {
        LOG_MSG("Read image failed\n");
        return GIF_ERROR;
    }

    printf("--- Frame %dx%d ---\n", frame->width, frame->height);
    uint8_t *debug_ptr = buffer;
    uint8_t *frame_ptr = frame->frame;
    for (int y=0; y<frame->height; y++) {
        for (int x=0; x<frame->width; x++) {
            uint8_t pattern_key = *debug_ptr;
            printf("%02x", pattern_key);
            frame_ptr[0] = gif->global_ct[pattern_key];
            frame_ptr[1] = gif->global_ct[pattern_key + gif->ct_size];
            frame_ptr[2] = gif->global_ct[pattern_key + gif->ct_size * 2];
            debug_ptr++;
            frame_ptr+=3;
        }
        printf("\n");
    }
    printf("--- End Frame ---\n");

    // Jump over the blocks we just parsed
    ptr++; // Skip keysize
    while (*ptr != 0) {  // TODO Danger bounds check
        uint8_t block_size = *ptr++;
        LOG_MSG("Skipping image block of size %d\n", block_size);
        ptr += block_size;
    }
    ptr++;

    int skip_subblock = 0;
    while (ptr < end_ptr && *ptr != BLOCK_IMAGE_DESCRIPTOR) {
        if (skip_subblock) {
            uint8_t block_size = *ptr;
            LOG_MSG("Subblock, size %d\n", *ptr);
            if (!block_size) {
                skip_subblock = 0;
                ptr++;
                continue;
            }
            ptr += block_size + 1;
            continue;
        }

        switch (*ptr) {
            case BLOCK_EXTENSION_INTRODUCER:
                LOG_MSG("Extension block, type 0x%02x, size %d\n", *(ptr+1), *(ptr+2));
                if (*(ptr+1) == 0xFF) {
                    LOG_MSG("Expect subblocks\n");
                    skip_subblock = 1;
                }

                // Skip the extension block
                ptr+= 3 + *(ptr+2);

                // Skip block terminator
                if (!skip_subblock) {
                    ptr++;
                }
                break;
            default:
                LOG_MSG("Not currently pointing to an image descriptor or a known block (0x%02x)\n", *ptr);
                return GIF_ERROR;
        }
    }

    if (ptr>= end_ptr || *ptr != BLOCK_IMAGE_DESCRIPTOR) {
        LOG_MSG("No image descriptor found (0x%02x)\n", *ptr);
        return GIF_ERROR;
    }

    gif->frame_ptr = ptr;

    return GIF_OK;
}


