// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#pragma once

#include <furi.h>
#include <gui/view_port.h>
#include <input/input.h>

typedef struct
{
    const char *title;
    const char *verb;
    const char *prompt;
    FuriMutex *mutex;
    char l1[28];
    char l2[28];
    char l3[28];
    char l4[28];
    bool busy;
    bool progress;
    uint32_t done;
    uint32_t total;
} ActionView;

typedef struct
{
    ActionView *view;
    ViewPort *view_port;
} ActionContext;

void action_view_draw (Canvas *canvas, void *context);
void action_view_input (InputEvent *event, void *context);
void action_view_set_lines (ActionContext *context, const char *line1,
                            const char *line2, const char *line3,
                            const char *line4);
void action_view_set_progress (ActionContext *context, const char *title,
                               uint32_t done, uint32_t total);
