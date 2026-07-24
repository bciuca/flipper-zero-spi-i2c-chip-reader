// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#include "dump_screens.h"

#include "action_view.h"
#include "i2c_memory.h"
#include "spi_memory.h"

#include <furi.h>
#include <gui/view_port.h>
#include <input/input.h>

typedef enum
{
    DumpStepIdle,
    DumpStepDetect,
    DumpStepDump,
} DumpStep;

static void
dump_screen_set_busy (ActionView *view, ViewPort *view_port, bool busy)
{
    furi_mutex_acquire (view->mutex, FuriWaitForever);
    view->busy = busy;
    furi_mutex_release (view->mutex);
    view_port_update (view_port);
}

static void
dump_screen_set_verb (ActionView *view, const char *verb)
{
    furi_mutex_acquire (view->mutex, FuriWaitForever);
    view->verb = verb;
    furi_mutex_release (view->mutex);
}

static void
dump_screen_free (Gui *gui, ActionView *view, ViewPort *view_port,
                  FuriMessageQueue *queue)
{
    gui_remove_view_port (gui, view_port);
    view_port_free (view_port);
    furi_message_queue_free (queue);
    furi_mutex_free (view->mutex);
}

void
dump_screen_run_spi (Gui *gui)
{
    ActionView view = {
        .title = "Dump Chip",
        .verb = "Detect",
        .prompt = "",
        .mutex = furi_mutex_alloc (FuriMutexTypeNormal),
    };

    FuriMessageQueue *queue = furi_message_queue_alloc (8, sizeof (InputEvent));
    ViewPort *view_port = view_port_alloc ();
    view_port_draw_callback_set (view_port, action_view_draw, &view);
    view_port_input_callback_set (view_port, action_view_input, queue);
    gui_add_view_port (gui, view_port, GuiLayerFullscreen);

    ActionContext context = {
        .view = &view,
        .view_port = view_port,
    };
    SpiChipKind chip_kind = SpiChipNone;
    DumpStep pending = DumpStepDetect;

    while (true)
        {
            if (pending != DumpStepIdle)
                {
                    dump_screen_set_busy (&view, view_port, true);

                    if (pending == DumpStepDetect)
                        {
                            char chip[28];
                            char wiring[28];
                            char status_register[28];
                            char jedec[28];
                            chip_kind = spi_memory_detect (
                                chip, wiring, status_register, jedec);
                            bool dumpable = chip_kind == SpiChipSst25vf040b
                                            || chip_kind == SpiChipEeprom;
                            dump_screen_set_verb (&view,
                                                  dumpable ? "Dump" : "Detect");
                            action_view_set_lines (&context, chip, wiring,
                                                   status_register, jedec);
                        }
                    else if (chip_kind == SpiChipSst25vf040b)
                        {
                            spi_memory_dump_flash (&context);
                        }
                    else if (chip_kind == SpiChipEeprom)
                        {
                            spi_memory_dump_eeprom (&context);
                        }

                    dump_screen_set_busy (&view, view_port, false);
                    pending = DumpStepIdle;
                    furi_message_queue_reset (queue);
                }

            InputEvent event;
            if (furi_message_queue_get (queue, &event, FuriWaitForever)
                != FuriStatusOk)
                continue;
            if (event.type != InputTypeShort)
                continue;

            if (event.key == InputKeyOk)
                {
                    pending = chip_kind == SpiChipSst25vf040b
                                      || chip_kind == SpiChipEeprom
                                  ? DumpStepDump
                                  : DumpStepDetect;
                }
            else if (event.key == InputKeyBack)
                {
                    break;
                }
        }

    dump_screen_free (gui, &view, view_port, queue);
}

void
dump_screen_run_i2c (Gui *gui)
{
    ActionView view = {
        .title = "Dump 24LC16B",
        .verb = "Detect",
        .prompt = "",
        .mutex = furi_mutex_alloc (FuriMutexTypeNormal),
    };

    FuriMessageQueue *queue = furi_message_queue_alloc (8, sizeof (InputEvent));
    ViewPort *view_port = view_port_alloc ();
    view_port_draw_callback_set (view_port, action_view_draw, &view);
    view_port_input_callback_set (view_port, action_view_input, queue);
    gui_add_view_port (gui, view_port, GuiLayerFullscreen);

    ActionContext context = {
        .view = &view,
        .view_port = view_port,
    };
    bool dumpable = false;
    DumpStep pending = DumpStepDetect;

    while (true)
        {
            if (pending != DumpStepIdle)
                {
                    dump_screen_set_busy (&view, view_port, true);

                    if (pending == DumpStepDetect)
                        {
                            char chip[28];
                            char address[28];
                            char mask[28];
                            dumpable = i2c_memory_detect (chip, address, mask);
                            dump_screen_set_verb (&view,
                                                  dumpable ? "Dump" : "Detect");
                            action_view_set_lines (&context, chip, address,
                                                   mask, NULL);
                        }
                    else
                        {
                            i2c_memory_dump (&context);
                        }

                    dump_screen_set_busy (&view, view_port, false);
                    pending = DumpStepIdle;
                    furi_message_queue_reset (queue);
                }

            InputEvent event;
            if (furi_message_queue_get (queue, &event, FuriWaitForever)
                != FuriStatusOk)
                continue;
            if (event.type != InputTypeShort)
                continue;

            if (event.key == InputKeyOk)
                {
                    pending = dumpable ? DumpStepDump : DumpStepDetect;
                }
            else if (event.key == InputKeyBack)
                {
                    break;
                }
        }

    dump_screen_free (gui, &view, view_port, queue);
}
