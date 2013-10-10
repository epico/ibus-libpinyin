/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (c) 2010 BYVoid <byvoid1@gmail.com>
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
#ifndef __PY_LIB_PINYIN_BOPOMOFO_ENGINE_H_
#define __PY_LIB_PINYIN_BOPOMOFO_ENGINE_H_

#include "PYEngine.h"
#include "PYPinyinProperties.h"

namespace PY {

class LibPinyinBopomofoEngine : public Engine {
public:
    LibPinyinBopomofoEngine (IBusEngine *engine);
    ~LibPinyinBopomofoEngine (void);

    // virtual functions
    gboolean processKeyEvent (guint keyval, guint keycode, guint modifiers);
    void focusIn (void);
    void focusOut (void);
#if IBUS_CHECK_VERSION (1, 5, 4)
    void setContentType (guint purpose, guint hints);
#endif
    void reset (void);
    void enable (void);
    void disable (void);
    void pageUp (void);
    void pageDown (void);
    void cursorUp (void);
    void cursorDown (void);
    gboolean propertyActivate (const gchar *prop_name, guint prop_state);
    void candidateClicked (guint index, guint button, guint state);

private:
    gboolean processPunct (guint keyval, guint keycode, guint modifiers);

private:
    void showSetupDialog (void);
    void connectEditorSignals (EditorPtr editor);

private:
    void commitText (Text & text);

private:
    PinyinProperties m_props;

#if IBUS_CHECK_VERSION (1, 5, 4)
    IBusInputPurpose m_input_purpose;
#endif

    guint m_prev_pressed_key;

    enum {
        MODE_INIT = 0,          // init mode
        MODE_PUNCT,             // punct mode
#if 0
        MODE_RAW,               // raw mode
        MODE_ENGLISH,           // press v into English input mode
        MODE_STROKE,            // press u into stroke input mode
        MODE_EXTENSION,         // press i into extension input mode
#endif
        MODE_LAST,
    } m_input_mode;

    EditorPtr m_editors[MODE_LAST];
    EditorPtr m_fallback_editor;
};

};


#endif
