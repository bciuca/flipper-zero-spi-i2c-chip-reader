// SPDX-License-Identifier: MIT
// Copyright (c) 2026, BC (https://github.com/bciuca).
//
// SPI/I2C serial-memory reader for Flipper Zero.
//
// Read-only protocol operations live in spi_memory.c and i2c_memory.c. The
// reusable full-screen views live in action_view.c and wiring_screen.c;
// dump_screens.c coordinates detection and dump actions. This file owns only
// application lifecycle and navigation.
//


#include "dump_screens.h"
#include "spi_i2c_chip_reader_icons.h"
#include "i2c_memory.h"
#include "spi_memory.h"
#include "wiring_screen.h"

#include <furi.h>
#include <gui/gui.h>
#include <gui/modules/submenu.h>
#include <gui/view_dispatcher.h>

#define VIEW_MENU 0u

typedef enum
{
    MenuSpiGroup,
    MenuI2cGroup,
    MenuSpiCheck,
    MenuSpiDump,
    MenuI2cCheck,
    MenuI2cDump,
} MenuId;

typedef enum
{
    MenuPageTop,
    MenuPageSpi,
    MenuPageI2c,
} MenuPage;

typedef struct
{
    Gui *gui;
    ViewDispatcher *view_dispatcher;
    Submenu *submenu;
    MenuPage page;
    int chosen;
    bool back;
} ChipReaderApp;

static void
chip_reader_menu_item_callback (void *context, uint32_t index)
{
    ChipReaderApp *app = context;
    app->chosen = (int)index;
    view_dispatcher_stop (app->view_dispatcher);
}

static bool
chip_reader_back_callback (void *context)
{
    ChipReaderApp *app = context;
    app->back = true;
    return false;
}

static void
chip_reader_build_menu (ChipReaderApp *app)
{
    submenu_reset (app->submenu);

    switch (app->page)
        {
        case MenuPageTop:
            submenu_set_header (app->submenu, "SPI-I2C Chip Reader");
            submenu_add_item (app->submenu, "SPI memory", MenuSpiGroup,
                              chip_reader_menu_item_callback, app);
            submenu_add_item (app->submenu, "I2C EEPROM", MenuI2cGroup,
                              chip_reader_menu_item_callback, app);
            break;
        case MenuPageSpi:
            submenu_set_header (app->submenu, "SPI memory");
            submenu_add_item (app->submenu, "Check wiring", MenuSpiCheck,
                              chip_reader_menu_item_callback, app);
            submenu_add_item (app->submenu, "Dump Chip", MenuSpiDump,
                              chip_reader_menu_item_callback, app);
            break;
        case MenuPageI2c:
            submenu_set_header (app->submenu, "I2C EEPROM");
            submenu_add_item (app->submenu, "Check wiring", MenuI2cCheck,
                              chip_reader_menu_item_callback, app);
            submenu_add_item (app->submenu, "Dump 24LC16B", MenuI2cDump,
                              chip_reader_menu_item_callback, app);
            break;
        }
}

int32_t
spi_i2c_chip_reader_app (void *context)
{
    UNUSED (context);

    ChipReaderApp *app = malloc (sizeof (ChipReaderApp));
    app->chosen = -1;
    app->back = false;
    app->page = MenuPageTop;
    app->gui = furi_record_open (RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc ();
    app->submenu = submenu_alloc ();

    view_dispatcher_set_event_callback_context (app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback (app->view_dispatcher,
                                                   chip_reader_back_callback);
    view_dispatcher_add_view (app->view_dispatcher, VIEW_MENU,
                              submenu_get_view (app->submenu));
    view_dispatcher_attach_to_gui (app->view_dispatcher, app->gui,
                                   ViewDispatcherTypeFullscreen);

    while (true)
        {
            app->chosen = -1;
            app->back = false;
            chip_reader_build_menu (app);
            view_dispatcher_switch_to_view (app->view_dispatcher, VIEW_MENU);
            view_dispatcher_run (app->view_dispatcher);

            if (app->back)
                {
                    if (app->page == MenuPageTop)
                        break;
                    app->page = MenuPageTop;
                    continue;
                }
            if (app->chosen < 0)
                break;

            switch (app->chosen)
                {
                case MenuSpiGroup:
                    app->page = MenuPageSpi;
                    break;
                case MenuI2cGroup:
                    app->page = MenuPageI2c;
                    break;
                case MenuSpiCheck:
                    wiring_screen_run (app->gui, &I_wiring_diagram,
                                       spi_memory_probe);
                    break;
                case MenuSpiDump:
                    dump_screen_run_spi (app->gui);
                    break;
                case MenuI2cCheck:
                    wiring_screen_run (app->gui, &I_wiring_diagram_i2c,
                                       i2c_memory_probe);
                    break;
                case MenuI2cDump:
                    dump_screen_run_i2c (app->gui);
                    break;
                default:
                    break;
                }
        }

    view_dispatcher_remove_view (app->view_dispatcher, VIEW_MENU);
    view_dispatcher_free (app->view_dispatcher);
    submenu_free (app->submenu);
    furi_record_close (RECORD_GUI);
    free (app);
    return 0;
}
