// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).

#include "action_view.h"

#include <gui/canvas.h>
#include <gui/elements.h>

void
action_view_draw (Canvas *canvas, void *context)
{
    ActionView *view = context;
    furi_mutex_acquire (view->mutex, FuriWaitForever);
    canvas_clear (canvas);
    canvas_set_font (canvas, FontPrimary);
    canvas_draw_str_aligned (canvas, 64, 0, AlignCenter, AlignTop, view->title);
    canvas_set_font (canvas, FontSecondary);

    if (view->busy && view->progress)
        {
            canvas_draw_str_aligned (canvas, 64, 20, AlignCenter, AlignTop,
                                     view->l1);

            char kilobytes[24];
            snprintf (kilobytes, sizeof (kilobytes), "%lu / %lu KB",
                      (unsigned long)(view->done / 1024u),
                      (unsigned long)(view->total / 1024u));
            canvas_draw_str_aligned (canvas, 64, 32, AlignCenter, AlignTop,
                                     kilobytes);

            const int32_t box_x = 8;
            const int32_t box_y = 46;
            const int32_t box_width = 112;
            const int32_t box_height = 10;
            canvas_draw_frame (canvas, box_x, box_y, box_width, box_height);

            uint32_t fill = view->total
                                ? (uint32_t)((uint64_t)view->done
                                             * (box_width - 4) / view->total)
                                : 0;
            if (fill)
                {
                    canvas_draw_box (canvas, box_x + 2, box_y + 2, fill,
                                     box_height - 4);
                }
        }
    else if (view->busy)
        {
            canvas_draw_str_aligned (canvas, 64, 28, AlignCenter, AlignTop,
                                     "Working...");
        }
    else if (view->l1[0] || view->l2[0] || view->l3[0] || view->l4[0])
        {
            if (view->l1[0])
                canvas_draw_str_aligned (canvas, 2, 12, AlignLeft, AlignTop,
                                         view->l1);
            if (view->l2[0])
                canvas_draw_str_aligned (canvas, 2, 22, AlignLeft, AlignTop,
                                         view->l2);
            if (view->l3[0])
                canvas_draw_str_aligned (canvas, 2, 32, AlignLeft, AlignTop,
                                         view->l3);
            if (view->l4[0])
                canvas_draw_str_aligned (canvas, 2, 42, AlignLeft, AlignTop,
                                         view->l4);
            elements_button_center (canvas, view->verb);
        }
    else
        {
            canvas_draw_str_aligned (canvas, 64, 26, AlignCenter, AlignTop,
                                     view->prompt);
            elements_button_center (canvas, view->verb);
        }

    furi_mutex_release (view->mutex);
}

void
action_view_input (InputEvent *event, void *context)
{
    furi_message_queue_put (context, event, 0);
}

void
action_view_set_lines (ActionContext *context, const char *line1,
                       const char *line2, const char *line3, const char *line4)
{
    ActionView *view = context->view;
    furi_mutex_acquire (view->mutex, FuriWaitForever);
    snprintf (view->l1, sizeof (view->l1), "%s", line1 ? line1 : "");
    snprintf (view->l2, sizeof (view->l2), "%s", line2 ? line2 : "");
    snprintf (view->l3, sizeof (view->l3), "%s", line3 ? line3 : "");
    snprintf (view->l4, sizeof (view->l4), "%s", line4 ? line4 : "");
    view->progress = false;
    furi_mutex_release (view->mutex);
    view_port_update (context->view_port);
}

void
action_view_set_progress (ActionContext *context, const char *title,
                          uint32_t done, uint32_t total)
{
    ActionView *view = context->view;
    furi_mutex_acquire (view->mutex, FuriWaitForever);
    snprintf (view->l1, sizeof (view->l1), "%s", title);
    view->progress = true;
    view->done = done;
    view->total = total;
    furi_mutex_release (view->mutex);
    view_port_update (context->view_port);
}
