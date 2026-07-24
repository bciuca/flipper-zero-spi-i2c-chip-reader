// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#include "i2c_memory.h"

#include <furi.h>
#include <furi_hal_i2c.h>
#include <storage/storage.h>

#define EEPROM_SIZE 2048u
#define EEPROM_BLOCK_SIZE 256u
#define EEPROM_CHUNK_SIZE 128u
#define EEPROM_ADDRESS(block) ((uint8_t)((0x50u | (block)) << 1))
#define EEPROM_PATH "/ext/24lc16b.bin"
#define I2C_READY_TIMEOUT_MS 5u
#define I2C_READ_TIMEOUT_MS 50u

static uint8_t
i2c_memory_scan (void)
{
    const FuriHalI2cBusHandle *i2c = &furi_hal_i2c_handle_external;
    furi_hal_i2c_acquire (i2c);

    uint8_t mask = 0;
    for (uint8_t block = 0; block < 8; block++)
        {
            if (furi_hal_i2c_is_device_ready (i2c, EEPROM_ADDRESS (block),
                                              I2C_READY_TIMEOUT_MS))
                {
                    mask |= (uint8_t)(1u << block);
                }
        }

    furi_hal_i2c_release (i2c);
    return mask;
}

bool
i2c_memory_detect (char chip[28], char address[28], char mask_line[28])
{
    uint8_t mask = i2c_memory_scan ();
    int responding_addresses = __builtin_popcount (mask);

    const char *name = responding_addresses == 8  ? "24LC16B"
                       : responding_addresses > 0 ? "24LCxx?"
                                                  : "none / wiring?";
    snprintf (chip, 28, "Chip: %s", name);
    snprintf (address, 28, "I2C 0x50-57: %d/8 ACK", responding_addresses);
    snprintf (mask_line, 28, "mask %02X", mask);
    FURI_LOG_I ("LC16", "scan mask=%02X n=%d -> %s", mask, responding_addresses,
                name);
    return responding_addresses == 8;
}

bool
i2c_memory_probe (void)
{
    return i2c_memory_scan () != 0;
}

void
i2c_memory_dump (ActionContext *context)
{
    const FuriHalI2cBusHandle *i2c = &furi_hal_i2c_handle_external;
    furi_hal_i2c_acquire (i2c);

    uint8_t mask = 0;
    for (uint8_t block = 0; block < 8; block++)
        {
            if (furi_hal_i2c_is_device_ready (i2c, EEPROM_ADDRESS (block),
                                              I2C_READY_TIMEOUT_MS))
                {
                    mask |= (uint8_t)(1u << block);
                }
        }
    if (mask != 0xFF)
        {
            furi_hal_i2c_release (i2c);
            char line2[28];
            snprintf (line2, sizeof (line2), "mask %02X (need FF)", mask);
            FURI_LOG_I ("LC16", "dump aborted: scan mask %02X", mask);
            action_view_set_lines (context, "Pin check FAILED", line2,
                                   "Check 0x50-0x57", "Not dumped");
            return;
        }

    uint8_t *data = malloc (EEPROM_SIZE);
    furi_check (data);
    memset (data, 0xA5, EEPROM_SIZE);

    bool success = true;
    for (uint8_t block = 0; block < 8 && success; block++)
        {
            for (uint16_t offset = 0; offset < EEPROM_BLOCK_SIZE && success;
                 offset += EEPROM_CHUNK_SIZE)
                {
                    success = furi_hal_i2c_read_mem (
                        i2c, EEPROM_ADDRESS (block), (uint8_t)offset,
                        data + block * EEPROM_BLOCK_SIZE + offset,
                        EEPROM_CHUNK_SIZE, I2C_READ_TIMEOUT_MS);
                }
        }
    furi_hal_i2c_release (i2c);

    bool changed = false;
    unsigned zeros = 0;
    for (unsigned i = 0; i < EEPROM_SIZE; i++)
        {
            if (data[i] != 0xA5)
                changed = true;
            if (data[i] == 0)
                zeros++;
        }

    Storage *storage = furi_record_open (RECORD_STORAGE);
    File *file = storage_file_alloc (storage);
    size_t bytes_written = 0;
    if (storage_file_open (file, EEPROM_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            bytes_written = storage_file_write (file, data, EEPROM_SIZE);
        }
    storage_file_close (file);
    storage_file_free (file);
    furi_record_close (RECORD_STORAGE);

    char line1[28];
    char line2[28];
    char line3[28];
    if (!success)
        {
            snprintf (line1, sizeof (line1), "I2C read error");
            snprintf (line2, sizeof (line2), "NAK mid-read.");
            snprintf (line3, sizeof (line3), "Check wiring.");
        }
    else if (!changed)
        {
            snprintf (line1, sizeof (line1), "No data read");
            snprintf (line2, sizeof (line2), "Bus didn't clock.");
            snprintf (line3, sizeof (line3), "Check wiring.");
        }
    else
        {
            snprintf (line1, sizeof (line1), "Done: wrote %u B",
                      (unsigned)bytes_written);
            snprintf (line2, sizeof (line2), "head %02X %02X %02X %02X",
                      data[0], data[1], data[2], data[3]);
            snprintf (line3, sizeof (line3), "Saved to SD card");
        }
    FURI_LOG_I (
        "LC16",
        "ok=%d changed=%d zeros=%u/2048 wrote=%u head %02X %02X %02X %02X",
        success, changed, zeros, (unsigned)bytes_written, data[0], data[1],
        data[2], data[3]);
    action_view_set_lines (context, line1, line2, line3, NULL);

    free (data);
}
