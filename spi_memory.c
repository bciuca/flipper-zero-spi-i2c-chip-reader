// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#include "spi_memory.h"

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_spi.h>
#include <storage/storage.h>

#define EEPROM_SIZE 4096u
#define EEPROM_PATH "/ext/eeprom_25aa32a.bin"

#define SST_SIZE 0x80000u
#define SST_CHUNK 4096u
#define SST_PATH "/ext/sst25vf040b.bin"

static bool
spi_command (const FuriHalSpiBusHandle *spi, uint8_t *tx, uint8_t *rx,
             size_t size)
{
    furi_hal_gpio_write (spi->cs, true);
    furi_delay_us (5);
    furi_hal_gpio_write (spi->cs, false);
    furi_delay_us (5);
    bool success = furi_hal_spi_bus_trx (spi, tx, rx, size, 3000);
    furi_hal_gpio_write (spi->cs, true);
    furi_delay_us (5);
    return success;
}

// Round-trip the volatile Write-Enable Latch to exercise every SPI signal
// without writing memory: RDSR -> WREN -> RDSR -> WRDI -> RDSR.
static bool
spi_wiring_test (const FuriHalSpiBusHandle *spi, uint8_t status_registers[3],
                 const char **verdict)
{
    uint8_t tx[2] = { 0 };
    uint8_t rx[2] = { 0 };
    bool success = true;

    tx[0] = 0x05;
    tx[1] = 0x00;
    success &= spi_command (spi, tx, rx, 2);
    status_registers[0] = rx[1];

    tx[0] = 0x06;
    success &= spi_command (spi, tx, rx, 1);

    tx[0] = 0x05;
    tx[1] = 0x00;
    success &= spi_command (spi, tx, rx, 2);
    status_registers[1] = rx[1];

    tx[0] = 0x04;
    success &= spi_command (spi, tx, rx, 1);

    tx[0] = 0x05;
    tx[1] = 0x00;
    success &= spi_command (spi, tx, rx, 2);
    status_registers[2] = rx[1];

    bool all_ff = status_registers[0] == 0xFF && status_registers[1] == 0xFF
                  && status_registers[2] == 0xFF;
    bool write_enable_set = (status_registers[1] & 0x02) != 0;
    bool write_enable_cleared = (status_registers[2] & 0x02) == 0;
    bool passed
        = success && !all_ff && write_enable_set && write_enable_cleared;

    *verdict = !success                ? "FAIL trx"
               : all_ff                ? "FAIL FF(SO?)"
               : !write_enable_set     ? "FAIL WEL0(SI?)"
               : !write_enable_cleared ? "FAIL WEL1(SO?)"
                                       : "OK";
    return passed;
}

SpiChipKind
spi_memory_detect (char chip[28], char wiring[28], char status_register[28],
                   char jedec[28])
{
    const FuriHalSpiBusHandle *spi = &furi_hal_spi_bus_handle_external;
    furi_hal_spi_acquire (spi);
    furi_hal_gpio_init (spi->cs, GpioModeOutputPushPull, GpioPullNo,
                        GpioSpeedVeryHigh);
    furi_hal_gpio_write (spi->cs, true);
    furi_delay_us (20);

    uint8_t id_tx[4] = { 0x9F, 0, 0, 0 };
    uint8_t id_rx[4] = { 0 };
    spi_command (spi, id_tx, id_rx, 4);

    uint8_t status_registers[3] = { 0 };
    const char *wiring_verdict = "?";
    bool wiring_passed
        = spi_wiring_test (spi, status_registers, &wiring_verdict);

    furi_hal_gpio_write (spi->cs, true);
    furi_hal_spi_release (spi);

    bool jedec_valid
        = !(id_rx[1] == 0x00 && id_rx[2] == 0x00 && id_rx[3] == 0x00)
          && !(id_rx[1] == 0xFF && id_rx[2] == 0xFF && id_rx[3] == 0xFF);
    bool is_sst = id_rx[1] == 0xBF && id_rx[2] == 0x25 && id_rx[3] == 0x8D;

    SpiChipKind kind = is_sst          ? SpiChipSst25vf040b
                       : jedec_valid   ? SpiChipOther
                       : wiring_passed ? SpiChipEeprom
                                       : SpiChipNone;
    const char *name = kind == SpiChipSst25vf040b ? "SST25VF040B"
                       : kind == SpiChipOther     ? "JEDEC flash"
                       : kind == SpiChipEeprom    ? "25xx EEPROM?"
                                                  : "none / wiring?";

    snprintf (chip, 28, "Chip: %s", name);
    snprintf (wiring, 28, "Wire: %s", wiring_verdict);
    snprintf (status_register, 28, "SR %02X>%02X>%02X", status_registers[0],
              status_registers[1], status_registers[2]);
    if (jedec_valid)
        {
            snprintf (jedec, 28, "JEDEC: %02X %02X %02X", id_rx[1], id_rx[2],
                      id_rx[3]);
        }
    else
        {
            snprintf (jedec, 28, "JEDEC: n/a");
        }

    FURI_LOG_I ("DETECT", "kind=%s wiring=%s %s jedec=%02X %02X %02X", name,
                wiring_passed ? "PASS" : "FAIL", status_register, id_rx[1],
                id_rx[2], id_rx[3]);
    return kind;
}

bool
spi_memory_probe (void)
{
    char chip[28];
    char wiring[28];
    char status_register[28];
    char jedec[28];
    return spi_memory_detect (chip, wiring, status_register, jedec)
           != SpiChipNone;
}

void
spi_memory_dump_eeprom (ActionContext *context)
{
    const FuriHalSpiBusHandle *spi = &furi_hal_spi_bus_handle_external;
    furi_hal_spi_acquire (spi);
    furi_hal_gpio_init (spi->cs, GpioModeOutputPushPull, GpioPullNo,
                        GpioSpeedVeryHigh);
    furi_hal_gpio_write (spi->cs, true);
    furi_delay_us (20);

    uint8_t status_registers[3] = { 0 };
    const char *wiring_verdict = "?";
    if (!spi_wiring_test (spi, status_registers, &wiring_verdict))
        {
            furi_hal_gpio_write (spi->cs, true);
            furi_hal_spi_release (spi);

            char line2[28];
            char line3[28];
            snprintf (line2, sizeof (line2), "Wire: %s", wiring_verdict);
            snprintf (line3, sizeof (line3), "SR %02X>%02X>%02X",
                      status_registers[0], status_registers[1],
                      status_registers[2]);
            FURI_LOG_I ("EE25", "dump aborted: wiring %s SR %02X %02X %02X",
                        wiring_verdict, status_registers[0],
                        status_registers[1], status_registers[2]);
            action_view_set_lines (context, "Pin check FAILED", line2, line3,
                                   "Not dumped");
            return;
        }

    size_t transfer_size = 3 + EEPROM_SIZE;
    uint8_t *tx = malloc (transfer_size);
    uint8_t *rx = malloc (transfer_size);
    furi_check (tx && rx);
    memset (tx, 0x00, transfer_size);
    memset (rx, 0xA5, transfer_size);
    tx[0] = 0x03;
    tx[1] = 0x00;
    tx[2] = 0x00;

    furi_hal_gpio_write (spi->cs, true);
    furi_delay_us (5);
    furi_hal_gpio_write (spi->cs, false);
    furi_delay_us (5);
    bool success = furi_hal_spi_bus_trx (spi, tx, rx, transfer_size, 3000);
    furi_hal_gpio_write (spi->cs, true);
    furi_hal_spi_release (spi);

    uint8_t *data = rx + 3;
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
            snprintf (line1, sizeof (line1), "SPI error");
            snprintf (line2, sizeof (line2), "No transfer.");
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
        "EE25",
        "ok=%d changed=%d zeros=%u/4096 wrote=%u head %02X %02X %02X %02X",
        success, changed, zeros, (unsigned)bytes_written, data[0], data[1],
        data[2], data[3]);
    action_view_set_lines (context, line1, line2, line3, NULL);

    free (tx);
    free (rx);
}

void
spi_memory_dump_flash (ActionContext *context)
{
    const FuriHalSpiBusHandle *spi = &furi_hal_spi_bus_handle_external;
    furi_hal_spi_acquire (spi);
    furi_hal_gpio_init (spi->cs, GpioModeOutputPushPull, GpioPullNo,
                        GpioSpeedVeryHigh);

    uint8_t id_tx[4] = { 0x9F, 0, 0, 0 };
    uint8_t id_rx[4] = { 0 };
    furi_hal_gpio_write (spi->cs, true);
    furi_delay_us (20);
    furi_hal_gpio_write (spi->cs, false);
    furi_delay_us (5);
    furi_hal_spi_bus_trx (spi, id_tx, id_rx, 4, 3000);
    furi_hal_gpio_write (spi->cs, true);

    if (!(id_rx[1] == 0xBF && id_rx[2] == 0x25 && id_rx[3] == 0x8D))
        {
            furi_hal_spi_release (spi);
            char line2[28];
            snprintf (line2, sizeof (line2), "JEDEC %02X %02X %02X", id_rx[1],
                      id_rx[2], id_rx[3]);
            FURI_LOG_I ("SST", "dump aborted: jedec %02X %02X %02X", id_rx[1],
                        id_rx[2], id_rx[3]);
            action_view_set_lines (context, "Pin check FAILED", line2,
                                   "expected BF 25 8D", "Not dumped");
            return;
        }

    uint8_t *tx = malloc (SST_CHUNK);
    uint8_t *rx = malloc (SST_CHUNK);
    furi_check (tx && rx);
    memset (tx, 0x00, SST_CHUNK);

    Storage *storage = furi_record_open (RECORD_STORAGE);
    File *file = storage_file_alloc (storage);

    uint8_t command_tx[4] = { 0x03, 0x00, 0x00, 0x00 };
    uint8_t command_rx[4];
    uint8_t head[4] = { 0 };
    uint32_t bytes_written = 0;
    bool non_ff = false;
    bool non_zero = false;
    bool spi_success = true;
    bool file_open = false;

    if (storage_file_open (file, SST_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            file_open = true;
            furi_hal_gpio_write (spi->cs, false);
            furi_delay_us (5);
            furi_hal_spi_bus_trx (spi, command_tx, command_rx, 4, 3000);

            for (uint32_t offset = 0; offset < SST_SIZE; offset += SST_CHUNK)
                {
                    if (!furi_hal_spi_bus_trx (spi, tx, rx, SST_CHUNK, 3000))
                        {
                            spi_success = false;
                            break;
                        }
                    if (offset == 0)
                        {
                            memcpy (head, rx, sizeof (head));
                        }
                    for (uint32_t i = 0; i < SST_CHUNK; i++)
                        {
                            if (rx[i] != 0xFF)
                                non_ff = true;
                            if (rx[i] != 0x00)
                                non_zero = true;
                        }

                    uint32_t written = storage_file_write (file, rx, SST_CHUNK);
                    bytes_written += written;
                    if (written != SST_CHUNK)
                        break;

                    uint32_t done = offset + SST_CHUNK;
                    if ((done % (SST_CHUNK * 16u)) == 0 || done >= SST_SIZE)
                        {
                            action_view_set_progress (
                                context, "Dumping SST25VF040B", done, SST_SIZE);
                        }
                }
            furi_hal_gpio_write (spi->cs, true);
        }

    furi_hal_spi_release (spi);
    storage_file_close (file);
    storage_file_free (file);
    furi_record_close (RECORD_STORAGE);

    const char *note = !file_open     ? "FILE-ERR"
                       : !spi_success ? "SPI-ERR"
                       : !non_ff      ? "ALL-FF"
                       : !non_zero    ? "ALL-00"
                                      : "OK";
    char line1[28];
    char line2[28];
    char line3[28];
    snprintf (line1, sizeof (line1), "Done %lu KB (%s)",
              (unsigned long)(bytes_written / 1024u), note);
    snprintf (line2, sizeof (line2), "JEDEC %02X %02X %02X", id_rx[1], id_rx[2],
              id_rx[3]);
    snprintf (line3, sizeof (line3), "head %02X %02X %02X %02X", head[0],
              head[1], head[2], head[3]);
    FURI_LOG_I (
        "SST",
        "wrote=%lu note=%s jedec=%02X %02X %02X head %02X %02X %02X %02X",
        (unsigned long)bytes_written, note, id_rx[1], id_rx[2], id_rx[3],
        head[0], head[1], head[2], head[3]);
    action_view_set_lines (context, line1, line2, line3, NULL);

    free (tx);
    free (rx);
}
