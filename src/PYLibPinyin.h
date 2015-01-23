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

#ifndef __PY_LIB_PINYIN_H_
#define __PY_LIB_PINYIN_H_

#include <memory>
#include <glib.h>

typedef struct _pinyin_context_t pinyin_context_t;
typedef struct _pinyin_instance_t pinyin_instance_t;

namespace PY {

class Config;

class LibPinyinBackEnd{

public:
    LibPinyinBackEnd ();
    virtual ~LibPinyinBackEnd ();

    gboolean setPinyinOptions (Config *config);
    gboolean setChewingOptions (Config *config);

    pinyin_context_t * initPinyinContext (Config *config);
    pinyin_context_t * initChewingContext (Config *config);

    pinyin_instance_t *allocPinyinInstance ();
    void freePinyinInstance (pinyin_instance_t *instance);
    pinyin_instance_t *allocChewingInstance ();
    void freeChewingInstance (pinyin_instance_t *instance);
    void modified (void);

    gboolean importPinyinDictionary (const char * filename);
    gboolean exportPinyinDictionary (const char * filename);
    gboolean clearPinyinUserData (const char * target);

    gboolean rememberUserInput (pinyin_instance_t * instance);

    /* use static initializer in C++. */
    static LibPinyinBackEnd & instance (void) { return *m_instance; }

    static void init (void);
    static void finalize (void);


private:
    gboolean saveUserDB (void);
    static gboolean timeoutCallback (gpointer data);

private:
    /* libpinyin context */
    pinyin_context_t *m_pinyin_context;
    pinyin_context_t *m_chewing_context;

    guint m_timeout_id;
    GTimer *m_timer;

private:
    static std::unique_ptr<LibPinyinBackEnd> m_instance;
};

};

#endif
