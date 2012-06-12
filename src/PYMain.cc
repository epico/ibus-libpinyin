/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <ibus.h>
#include <stdlib.h>
#include <locale.h>
#include "PYEngine.h"
#include "PYPointer.h"
#include "PYBus.h"
#include "PYConfig.h"
#include "PYPConfig.h"
#ifdef IBUS_BUILD_LIBPINYIN
#include "PYLibPinyin.h"
#endif

using namespace PY;

#define N_(text) text

static Pointer<IBusFactory> factory;

/* options */
static gboolean ibus = FALSE;
static gboolean verbose = FALSE;

static void
show_version_and_quit (void)
{
    g_print ("%s - Version %s\n", g_get_application_name (), VERSION);
    exit (EXIT_SUCCESS);
}

static const GOptionEntry entries[] =
{
    { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
        (gpointer) show_version_and_quit, "Show the application's version.", NULL },
    { "ibus",    'i', 0, G_OPTION_ARG_NONE, &ibus, "component is executed by ibus", NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "verbose", NULL },
    { NULL },
};


static void
ibus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    g_debug ("bus disconnected");
    ibus_quit ();
}


static void
start_component (void)
{
    Pointer<IBusComponent> component;

    ibus_init ();
    Bus bus;

    if (!bus.isConnected ()) {
        g_warning ("Can not connect to ibus!");
        exit (0);
    }

    if (!ibus_bus_get_config (bus)) {
        g_warning ("IBus config component is not ready!");
        exit (0);
    }

#ifdef IBUS_BUILD_LIBPINYIN
    LibPinyinBackEnd::init ();
#endif

#ifdef IBUS_BUILD_LIBPINYIN
    LibPinyinPinyinConfig::init (bus);
    LibPinyinBopomofoConfig::init (bus);
#endif

    g_signal_connect ((IBusBus *)bus, "disconnected", G_CALLBACK (ibus_disconnected_cb), NULL);

    component = ibus_component_new ("org.freedesktop.IBus.Libpinyin",
                                    N_("Libpinyin input method"),
                                    VERSION,
                                    "GPL",
                                    "Peng Wu <alexepico@gmail.com>",
                                    "https://github.com/libpinyin/ibus-libpinyin",
                                    "",
                                    "ibus-libpinyin");
#ifdef IBUS_BUILD_LIBPINYIN
    ibus_component_add_engine (component,
                               ibus_engine_desc_new ("libpinyin-debug",
                                                     N_("Intelligent Pinyin (debug)"),
                                                     N_("Intelligent Pinyin input method (debug)"),
                                                     "zh_CN",
                                                     "GPL",
                                                     "Peng Huang <shawn.p.huang@gmail.com>\n"
                                                     "Peng Wu <alexepico@gmail.com>\n"
                                                     "BYVoid <byvoid1@gmail.com>",
                                                     PKGDATADIR "/icons/ibus-pinyin.svg",
                                                     "us"));
    ibus_component_add_engine (component,
                               ibus_engine_desc_new ("libbopomofo-debug",
                                                     N_("Intelligent Bopomofo (debug)"),
                                                     N_("Intelligent Bopomofo input method (debug)"),
                                                     "zh_CN",
                                                     "GPL",
                                                     "BYVoid <byvoid1@gmail.com>\n"
                                                     "Peng Wu <alexepico@gmail.com>\n"
                                                     "Peng Huang <shawn.p.huang@gmail.com>",
                                                     PKGDATADIR "/icons/ibus-bopomofo.svg",
                                                     "us"));
#endif

    factory = ibus_factory_new (ibus_bus_get_connection (bus));

    if (ibus) {
#ifdef IBUS_BUILD_LIBPINYIN
        ibus_factory_add_engine (factory, "libpinyin", IBUS_TYPE_PINYIN_ENGINE);
        ibus_factory_add_engine (factory, "libbopomofo", IBUS_TYPE_PINYIN_ENGINE);
#endif
        ibus_bus_request_name (bus, "org.freedesktop.IBus.Libpinyin", 0);
    }
    else {
#ifdef IBUS_BUILD_LIBPINYIN
        ibus_factory_add_engine (factory, "libpinyin-debug", IBUS_TYPE_PINYIN_ENGINE);
        ibus_factory_add_engine (factory, "libbopomofo-debug", IBUS_TYPE_PINYIN_ENGINE);
#endif
        ibus_bus_register_component (bus, component);
    }

    ibus_main ();
}

#include <signal.h>

static void
sigterm_cb (int sig)
{
#ifdef IBUS_BUILD_LIBPINYIN
    LibPinyinBackEnd::finalize ();
#endif
    ::exit (EXIT_FAILURE);
}

static void
atexit_cb (void)
{
#ifdef IBUS_BUILD_LIBPINYIN
    LibPinyinBackEnd::finalize ();
#endif
}

int
main (gint argc, gchar **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    setlocale (LC_ALL, "");

    context = g_option_context_new ("- ibus pinyin engine component");

    g_option_context_add_main_entries (context, entries, "ibus-libpinyin");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message);
        exit (-1);
    }

    ::signal (SIGTERM, sigterm_cb);
    ::signal (SIGINT, sigterm_cb);
    g_atexit (atexit_cb);

    start_component ();
    return 0;
}
