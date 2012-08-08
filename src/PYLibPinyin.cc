/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2011 Peng Wu <alexepico@gmail.com>
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

#include "PYLibPinyin.h"
#include <pinyin.h>
#include "PYPConfig.h"

#define LIBPINYIN_SAVE_TIMEOUT   (5 * 60)

using namespace PY;

std::unique_ptr<LibPinyinBackEnd> LibPinyinBackEnd::m_instance;

static LibPinyinBackEnd libpinyin_backend;

LibPinyinBackEnd::LibPinyinBackEnd () {
    m_timeout_id = 0;
    m_timer = g_timer_new();
    m_pinyin_context = NULL;
    m_chewing_context = NULL;
}

LibPinyinBackEnd::~LibPinyinBackEnd () {
    g_timer_destroy (m_timer);
    if (m_timeout_id != 0) {
        saveUserDB ();
        g_source_remove (m_timeout_id);
    }

    if (m_pinyin_context)
        pinyin_fini(m_pinyin_context);
    m_pinyin_context = NULL;
    if (m_chewing_context)
        pinyin_fini(m_chewing_context);
    m_chewing_context = NULL;
}

pinyin_context_t *
LibPinyinBackEnd::initPinyinContext (Config *config)
{
    pinyin_context_t * context = NULL;

    gchar * userdir = g_build_filename (g_get_home_dir(), ".cache",
                                        "ibus", "libpinyin", NULL);
    int retval = g_mkdir_with_parents (userdir, 0700);
    if (retval) {
        g_free(userdir); userdir = NULL;
    }
    context = pinyin_init ("/usr/share/libpinyin/data", userdir);
    g_free (userdir);

    const char *dicts = config->dictionaries ().c_str ();
    gchar ** indices = g_strsplit_set (dicts, "", -1);
    for (size_t i = 0; i < g_strv_length(indices); ++i) {
        int index = atoi (indices [i]);
        if (index <= 1)
            continue;

        pinyin_load_phrase_library (context, index);
    }
    g_strfreev (indices);

    return context;
}

pinyin_instance_t *
LibPinyinBackEnd::allocPinyinInstance ()
{
    Config * config = &LibPinyinPinyinConfig::instance ();
    if (NULL == m_pinyin_context) {
        m_pinyin_context = initPinyinContext (config);
    }

    setPinyinOptions (config);
    return pinyin_alloc_instance (m_pinyin_context);
}

void
LibPinyinBackEnd::freePinyinInstance (pinyin_instance_t *instance)
{
    pinyin_free_instance (instance);
}

pinyin_context_t *
LibPinyinBackEnd::initChewingContext (Config *config)
{
    pinyin_context_t * context = NULL;

    gchar * userdir = g_build_filename (g_get_home_dir(), ".cache",
                                        "ibus", "libbopomofo", NULL);
    int retval = g_mkdir_with_parents (userdir, 0700);
    if (retval) {
        g_free(userdir); userdir = NULL;
    }
    context = pinyin_init ("/usr/share/libpinyin/data", userdir);
    g_free(userdir);

    const char *dicts = config->dictionaries ().c_str ();
    gchar ** indices = g_strsplit_set (dicts, "", -1);
    for (size_t i = 0; i < g_strv_length(indices); ++i) {
        int index = atoi (indices [i]);
        if (index <= 1)
            continue;

        pinyin_load_phrase_library (context, index);
    }
    g_strfreev (indices);

    return context;
}

pinyin_instance_t *
LibPinyinBackEnd::allocChewingInstance ()
{
    Config *config = &LibPinyinBopomofoConfig::instance ();
    if (NULL == m_chewing_context) {
        m_chewing_context = initChewingContext (config);
    }

    setChewingOptions (config);
    return pinyin_alloc_instance (m_chewing_context);
}

void
LibPinyinBackEnd::freeChewingInstance (pinyin_instance_t *instance)
{
    pinyin_free_instance (instance);
}

void
LibPinyinBackEnd::init (void) {
    g_assert (NULL == m_instance.get ());
    LibPinyinBackEnd * backend = new LibPinyinBackEnd;
    m_instance.reset (backend);
}

void
LibPinyinBackEnd::finalize (void) {
    m_instance.reset ();
}

/* Here are the double pinyin keyboard scheme mapping table. */
static const struct{
    gint double_pinyin_keyboard;
    DoublePinyinScheme scheme;
} double_pinyin_options [] = {
    {0, DOUBLE_PINYIN_MS},
    {1, DOUBLE_PINYIN_ZRM},
    {2, DOUBLE_PINYIN_ABC},
    {3, DOUBLE_PINYIN_ZIGUANG},
    {4, DOUBLE_PINYIN_PYJJ},
    {5, DOUBLE_PINYIN_XHE}
};

gboolean
LibPinyinBackEnd::setPinyinOptions (Config *config)
{
    if (NULL == m_pinyin_context)
        return FALSE;

    const gint map = config->doublePinyinSchema ();
    for (guint i = 0; i < G_N_ELEMENTS (double_pinyin_options); i++) {
        if (map == double_pinyin_options[i].double_pinyin_keyboard) {
            /* set double pinyin scheme. */
            DoublePinyinScheme scheme = double_pinyin_options[i].scheme;
            pinyin_set_double_pinyin_scheme (m_pinyin_context, scheme);
        }
    }

    pinyin_option_t options = config->option()
        | USE_RESPLIT_TABLE | USE_DIVIDED_TABLE;
    pinyin_set_options (m_pinyin_context, options);
    return TRUE;
}

/* Here are the chewing keyboard scheme mapping table. */
static const struct {
    gint bopomofo_keyboard;
    ChewingScheme scheme;
} chewing_options [] = {
    {0, CHEWING_STANDARD},
    {1, CHEWING_GINYIEH},
    {2, CHEWING_ETEN},
    {3, CHEWING_IBM}
};


gboolean
LibPinyinBackEnd::setChewingOptions (Config *config)
{
    if (NULL == m_chewing_context)
        return FALSE;

    const gint map = config->bopomofoKeyboardMapping ();
    for (guint i = 0; i < G_N_ELEMENTS (chewing_options); i++) {
        if (map == chewing_options[i].bopomofo_keyboard) {
            /* TODO: set chewing scheme. */
            ChewingScheme scheme = chewing_options[i].scheme;
            pinyin_set_chewing_scheme (m_chewing_context, scheme);
        }
    }

    pinyin_option_t options = config->option() | USE_TONE;
    pinyin_set_options(m_chewing_context, options);
    return TRUE;
}

void
LibPinyinBackEnd::modified (void)
{
    /* Restart the timer */
    g_timer_start (m_timer);

    if (m_timeout_id != 0)
        return;

    m_timeout_id = g_timeout_add_seconds (LIBPINYIN_SAVE_TIMEOUT,
                                          LibPinyinBackEnd::timeoutCallback,
                                          static_cast<gpointer> (this));
}

gboolean
LibPinyinBackEnd::timeoutCallback (gpointer data)
{
    LibPinyinBackEnd *self = static_cast<LibPinyinBackEnd *> (data);

    /* Get the elapsed time since last modification of database. */
    guint elapsed = (guint)g_timer_elapsed (self->m_timer, NULL);

    if (elapsed >= LIBPINYIN_SAVE_TIMEOUT &&
        self->saveUserDB ()) {
        self->m_timeout_id = 0;
        return FALSE;
    }

    return TRUE;
}

gboolean
LibPinyinBackEnd::saveUserDB (void)
{
    if (m_pinyin_context)
        pinyin_save (m_pinyin_context);
    if (m_chewing_context)
        pinyin_save (m_chewing_context);
    return TRUE;
}
