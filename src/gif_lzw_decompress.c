//
// Created by Hugo Trippaers on 23/07/2023.
//

#include <stdint.h>
#include <stdio.h>
#include "gif_lzw_decompress.h"

#define DEBUG 1

#if DEBUG
#define LOG_MSG printf
#else
#define LOG_MSG(...)
#endif

typedef struct {
    uint16_t length;
    uint16_t prefix;
    uint8_t  suffix;
} entry_t;

typedef struct {
    uint8_t code_size;
    uint8_t bits_remaining;
    uint16_t current;
    uint8_t *data_ptr;
    uint16_t entries;
    entry_t table[4096];
    uint8_t *frame;
} reader_state_t;

static void init_table(reader_state_t *reader_state, uint16_t key_size);
static void add_table_entry(reader_state_t *reader_state, uint16_t length, uint16_t prefix, uint8_t suffix);
static uint16_t read_bits(reader_state_t *reader_state);

gif_lzw_error_t gif_decoder_read_image_data(uint8_t *data, uint8_t *buffer) {
    uint8_t *ptr = data;
    uint8_t lzw_min_code_size = *ptr;

    if (lzw_min_code_size < 2 || lzw_min_code_size > 8) {
        LOG_MSG("Invalid LZW minimum code size %d\n", lzw_min_code_size);
        return GIF_LZW_ERROR;
    }
    ptr++;

    uint8_t clear_code = 1 << lzw_min_code_size;
    uint8_t stop_code = clear_code + 1;
    uint8_t block_size = *ptr;

    LOG_MSG("LZW minimum code size %d\n", lzw_min_code_size);
    LOG_MSG("First block %d\n", block_size);

    // Read bit by bit, from right to left
    // the first code to read is lzw_min_code_size + 1
    // if the code read is the clear_code we clear the compression table
    // if the code read is the stop_code we are done

    reader_state_t reader_state = {
            .code_size = lzw_min_code_size + 1,
            .bits_remaining = 0,
            .data_ptr = ptr + 1
    };
    init_table(&reader_state, lzw_min_code_size);

    uint16_t pattern_length = 0;
    entry_t *current_entry = &reader_state.table[0];
    uint8_t *buffer_ptr = buffer;
    while (1) {
        uint16_t next_code = read_bits(&reader_state);
        LOG_MSG("Read %04x, (bits_remaining %d)\n", next_code, reader_state.bits_remaining);

        if (next_code == clear_code) {
            LOG_MSG("Cleaning table and resetting key size after clear code\n");
            init_table(&reader_state, lzw_min_code_size);
            reader_state.code_size = lzw_min_code_size + 1;
            current_entry = &reader_state.table[0];
            continue;
        }

        if (next_code == stop_code) {
            LOG_MSG("Stop code\n");
            break;
        }

        if (next_code == (1 << reader_state.code_size) - 1) {
            reader_state.code_size++;
        }

        current_entry = &reader_state.table[next_code];
        pattern_length = current_entry->length;
        add_table_entry(&reader_state, pattern_length + 1, next_code, current_entry->suffix);

        for (int i = 0; i < pattern_length; i++) {
            LOG_MSG("Writing %02x at %08x\n", current_entry->suffix, (unsigned int)(buffer_ptr + current_entry->length - 1));
            *(buffer_ptr + current_entry->length - 1) = current_entry->suffix;
            if (current_entry->prefix == 0xFFF)
                break;
            else
                current_entry = &reader_state.table[current_entry->prefix];
        }
        buffer_ptr += pattern_length;
    }

    return GIF_LZW_OK;
}

static void init_table(reader_state_t *reader_state, uint16_t key_size) {
    reader_state->entries = (1 << key_size) + 2;
    for (int i=0; i<reader_state->entries; i++) {
        reader_state->table[i].length = 1;
        reader_state->table[i].prefix = 0xFFF;
        reader_state->table[i].suffix = i;
    }
}

static void add_table_entry(reader_state_t *reader_state, uint16_t length, uint16_t prefix, uint8_t suffix) {
    LOG_MSG("Adding entry %d, length=%d, prefix=%04x, suffix=%02x\n",
            reader_state->entries + 1, length, prefix, suffix);
    reader_state->table[reader_state->entries].length = length;
    reader_state->table[reader_state->entries].prefix = prefix;
    reader_state->table[reader_state->entries].suffix = suffix;
    reader_state->entries++;
}

static uint16_t read_bits(reader_state_t *reader_state) {
    uint8_t *ptr = reader_state->data_ptr;
    // TODO check for block end

    if (reader_state->bits_remaining == 0) {
        reader_state->current = *ptr | *(ptr + 1) << 8;
        reader_state->bits_remaining += 16;
        reader_state->data_ptr = ptr+2;
    }

    if (reader_state->bits_remaining < 8) {
        // Add 8 new bits of data to the reader
        // by moving the data pointer forward by one position
        // and filling up with a new byte
        reader_state->current >>= 8;
        reader_state->current |= *(ptr + 1) << 8;
        reader_state->bits_remaining += 8;
        reader_state->data_ptr = ptr+1;
    }

    if (reader_state->bits_remaining < reader_state->code_size) {
        LOG_MSG("Not enough data remaining\n");
        return 0;
    }

    uint16_t value = reader_state->current;
    value >>= (16 - reader_state->bits_remaining);
    value &= (1 << reader_state->code_size) - 1;

    reader_state->bits_remaining -= reader_state->code_size;

    return value;
}
