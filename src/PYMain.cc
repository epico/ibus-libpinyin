/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <ibus.h>
#include <stdlib.h>
#include <locale.h>
#include <libintl.h>
#include "PYEngine.h"
#include "PYPointer.h"
#include "PYBus.h"
#include "PYConfig.h"
#include "PYPConfig.h"
#include "PYLibPinyin.h"
#ifdef IBUS_BUILD_ENGLISH_INPUT_MODE
#include "PYEnglishDatabase.h"
#endif
#ifdef IBUS_BUILD_TABLE_INPUT_MODE
#include "PYTableDatabase.h"
#endif
#include "PYXMLUtil.h"

using namespace PY;

#define N_(text) text

static Pointer<IBusFactory> factory;

/* options */
static gboolean ibus = FALSE;
static gboolean xml = FALSE;
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
    { "xml",     'x', 0, G_OPTION_ARG_NONE, &xml, "list engines", NULL },
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

    LibPinyinBackEnd::init ();

    PinyinConfig::init ();
    BopomofoConfig::init ();

#ifdef IBUS_BUILD_ENGLISH_INPUT_MODE
    EnglishDatabase::init ();
#endif

#ifdef IBUS_BUILD_TABLE_INPUT_MODE
    TableDatabase::init ();
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
                                                     N_("Bopomofo (debug)"),
                                                     N_("Bopomofo input method (debug)"),
                                                     "zh_TW",
                                                     "GPL",
                                                     "BYVoid <byvoid1@gmail.com>\n"
                                                     "Peng Wu <alexepico@gmail.com>\n"
                                                     "Peng Huang <shawn.p.huang@gmail.com>",
                                                     PKGDATADIR "/icons/ibus-bopomofo.svg",
                                                     "us"));

    factory = ibus_factory_new (ibus_bus_get_connection (bus));

    if (ibus) {
        ibus_factory_add_engine (factory, "libpinyin", IBUS_TYPE_PINYIN_ENGINE);
        ibus_factory_add_engine (factory, "libbopomofo", IBUS_TYPE_PINYIN_ENGINE);
        ibus_bus_request_name (bus, "org.freedesktop.IBus.Libpinyin", 0);
    }
    else {
        ibus_factory_add_engine (factory, "libpinyin-debug", IBUS_TYPE_PINYIN_ENGINE);
        ibus_factory_add_engine (factory, "libbopomofo-debug", IBUS_TYPE_PINYIN_ENGINE);
        ibus_bus_register_component (bus, component);
    }

    ibus_main ();
}

#include <signal.h>

static void
sigterm_cb (int sig)
{
    LibPinyinBackEnd::finalize ();

    ::exit (EXIT_FAILURE);
}

static void
atexit_cb (void)
{
    LibPinyinBackEnd::finalize ();
}

static void
print_engine_xml (void)
{
    gboolean success = FALSE;
    gchar * content = NULL;

    /* check the user engines.xml first. */
    gchar * user_config = g_build_filename (g_get_user_config_dir (),
                                            "ibus", "libpinyin", "engines.xml", NULL);
    gchar * system_config = g_build_filename (PKGDATADIR, "default.xml", NULL);

    /* if not, print the default.xml and exit. */
    if (!g_file_test (user_config, G_FILE_TEST_IS_REGULAR)) {
        if (!g_file_test (system_config, G_FILE_TEST_IS_REGULAR)) {
            g_free (system_config);
            g_free (user_config);
            return;
        }
        content = load_file_content (system_config);
        printf ("%s", content);
        g_free (content);
        g_free (system_config);
        g_free (user_config);

        return;
    }

    /* if the engines.xml exists, compare the version of two xml files. */
    gchar * system_version = NULL;
    success = parse_engine_version (system_config, &system_version);
    gchar * user_version = NULL;
    success = success && parse_engine_version (user_config, &user_version);
    success = success && 0 == g_strcmp0 (system_version, user_version);
    g_free (system_version);
    g_free (user_version);
    if (success) {
        content = load_file_content (user_config);
        printf ("%s", content);
        g_free (content);
        g_free (system_config);
        g_free (user_config);
        return;
    }

    /* if the version mis-match, create the new engines.xml by ibus-setup-libpinyin. */
    g_spawn_command_line_sync (LIBEXECDIR"/ibus-setup-libpinyin resync-engine",
                               NULL, NULL, NULL, NULL);

    /* print the user engines.xml. */
    if (g_file_test (user_config, G_FILE_TEST_IS_REGULAR)) {
        content = load_file_content (user_config);
        printf ("%s", content);
        g_free (content);
        g_free (system_config);
        g_free (user_config);
        return;
    }
}

int
main (gint argc, gchar **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    setlocale (LC_ALL, "");

    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    context = g_option_context_new ("- ibus pinyin engine component");

    g_option_context_add_main_entries (context, entries, "ibus-libpinyin");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message);
        exit (-1);
    }

    if (xml) {
      print_engine_xml ();
      return 0;
    }

    ::signal (SIGTERM, sigterm_cb);
    ::signal (SIGINT, sigterm_cb);
    atexit (atexit_cb);

    start_component ();
    return 0;
}
