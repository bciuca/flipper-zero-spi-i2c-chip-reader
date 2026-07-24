// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#pragma once

#include "action_view.h"

typedef enum
{
    SpiChipNone,
    SpiChipOther,
    SpiChipEeprom,
    SpiChipSst25vf040b,
} SpiChipKind;

SpiChipKind spi_memory_detect (char chip[28], char wiring[28],
                               char status_register[28], char jedec[28]);
bool spi_memory_probe (void);
void spi_memory_dump_eeprom (ActionContext *context);
void spi_memory_dump_flash (ActionContext *context);
