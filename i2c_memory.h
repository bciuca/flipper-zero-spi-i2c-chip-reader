// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#pragma once

#include "action_view.h"

bool i2c_memory_detect (char chip[28], char address[28], char mask[28]);
bool i2c_memory_probe (void);
void i2c_memory_dump (ActionContext *context);
