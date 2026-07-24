// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#include "wiring_screen.h"

#include "action_view.h"

#include <furi.h>
#include <gui/canvas.h>
#include <gui/view_port.h>
#include <input/input.h>

typedef enum
{
    WiringStateIdle,
    WiringStateBusy,
    WiringStateOk,
    WiringStateFault,
} WiringState;

typedef struct
{
    FuriMutex *mutex;
    WiringState state;
    const Icon *icon;
    bool (*probe) (void);
} WiringView;

static void
wiring_screen_draw (Canvas *canvas, void *context)
{
    WiringView *view = context;
    furi_mutex_acquire (view->mutex, FuriWaitForever);
    WiringState state = view->state;
    furi_mutex_release (view->mutex);

    canvas_clear (canvas);
    canvas_draw_icon (canvas, 0, 0, view->icon);
    canvas_set_font (canvas, FontSecondary);

    const int32_t radius = 3;
    const int32_t hint_text_x = 2 + (2 * radius + 1) + 3;
    const char *status = state == WiringStateBusy    ? "TESTING..."
                         : state == WiringStateOk    ? "WIRING: OK"
                         : state == WiringStateFault ? "WIRING: NO CHIP"
                                                     : "TO TEST";
    int32_t text_x = state == WiringStateIdle ? hint_text_x : 2;
    int32_t badge_width = text_x + canvas_string_width (canvas, status) + 2;
    if (badge_width < 43)
        badge_width = 43;

    canvas_set_color (canvas, ColorBlack);
    canvas_draw_box (canvas, 0, 0, badge_width, 11);
    canvas_set_color (canvas, ColorWhite);
    if (state == WiringStateIdle)
        {
            canvas_draw_disc (canvas, 2 + radius, 5, radius);
        }
    canvas_draw_str_aligned (canvas, text_x, 2, AlignLeft, AlignTop, status);
    canvas_set_color (canvas, ColorBlack);
}

void
wiring_screen_run (Gui *gui, const Icon *icon, bool (*probe) (void))
{
    WiringView view = {
        .mutex = furi_mutex_alloc (FuriMutexTypeNormal),
        .state = WiringStateIdle,
        .icon = icon,
        .probe = probe,
    };

    FuriMessageQueue *queue = furi_message_queue_alloc (8, sizeof (InputEvent));
    ViewPort *view_port = view_port_alloc ();
    view_port_draw_callback_set (view_port, wiring_screen_draw, &view);
    view_port_input_callback_set (view_port, action_view_input, queue);
    gui_add_view_port (gui, view_port, GuiLayerFullscreen);

    bool should_probe = false;
    while (true)
        {
            if (should_probe)
                {
                    furi_mutex_acquire (view.mutex, FuriWaitForever);
                    view.state = WiringStateBusy;
                    furi_mutex_release (view.mutex);
                    view_port_update (view_port);

                    bool responded = view.probe ();

                    furi_mutex_acquire (view.mutex, FuriWaitForever);
                    view.state = responded ? WiringStateOk : WiringStateFault;
                    furi_mutex_release (view.mutex);
                    view_port_update (view_port);

                    should_probe = false;
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
                    should_probe = true;
                }
            else if (event.key == InputKeyBack)
                {
                    break;
                }
        }

    gui_remove_view_port (gui, view_port);
    view_port_free (view_port);
    furi_message_queue_free (queue);
    furi_mutex_free (view.mutex);
}
