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

#include "PYPDoublePinyinEditor.h"
#include "PYConfig.h"
#include "PYLibPinyin.h"

using namespace PY;

/*
 * c in 'a' ... 'z' => id = c - 'a'
 * c == ';'         => id = 26
 * else             => id = -1
 */
#define ID(c) \
    ((c >= IBUS_a && c <= IBUS_z) ? c - IBUS_a : (c == IBUS_semicolon ? 26 : -1))

#define IS_ALPHA(c) \
        ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))


LibPinyinDoublePinyinEditor::LibPinyinDoublePinyinEditor
( PinyinProperties & props, Config & config)
    : LibPinyinPinyinEditor (props, config)
{
    m_instance = LibPinyinBackEnd::instance ().allocPinyinInstance ();
}

LibPinyinDoublePinyinEditor::~LibPinyinDoublePinyinEditor (void)
{
    LibPinyinBackEnd::instance ().freePinyinInstance (m_instance);
    m_instance = NULL;
}

gboolean
LibPinyinDoublePinyinEditor::insert (gint ch)
{
    /* is full */
    if (G_UNLIKELY (m_text.length () >= MAX_PINYIN_LEN))
        return TRUE;

    gint id = ID (ch);
    if (id == -1) {
        /* it is not available ch */
        return FALSE;
    }

#if 0
    if (G_UNLIKELY (m_text.empty () && ID_TO_SHENG (id) == PINYIN_ID_VOID)) {
        return FALSE;
    }
#endif

    m_text.insert (m_cursor++, ch);
    updatePinyin ();
    update ();

    return TRUE;
}

void LibPinyinDoublePinyinEditor::reset (void)
{
    LibPinyinPinyinEditor::reset ();
}

gboolean
LibPinyinDoublePinyinEditor::processKeyEvent (guint keyval, guint keycode,
                                              guint modifiers)
{
    /* handle ';' key */
    if (G_UNLIKELY (keyval == IBUS_semicolon)) {
        if (cmshm_filter (modifiers) == 0) {

            if (m_text.empty ())
                return FALSE;

            if (insert (keyval))
                return TRUE;
        }
    }

    return LibPinyinPinyinEditor::processKeyEvent (keyval, keycode, modifiers);
}

void
LibPinyinDoublePinyinEditor::updatePinyin (void)
{
    if (G_UNLIKELY (m_text.empty ())) {
        m_pinyin_len = 0;
        /* TODO: check whether to replace "" with NULL. */
        pinyin_parse_more_double_pinyins (m_instance, "");
        pinyin_guess_sentence(m_instance);
        return;
    }

    m_pinyin_len =
        pinyin_parse_more_double_pinyins (m_instance, m_text.c_str ());
    pinyin_guess_sentence (m_instance);
}


void
LibPinyinDoublePinyinEditor::updateAuxiliaryText (void)
{
    if (G_UNLIKELY (m_text.empty ())) {
        hideAuxiliaryText ();
        return;
    }

    m_buffer.clear ();

    guint len = 0;
    pinyin_get_n_pinyin (m_instance, &len);

    for (guint i = 0; i < len; ++i) {
        PinyinKey *key = NULL;
        pinyin_get_pinyin_key (m_instance, i, &key);

        PinyinKeyPos *pos = NULL;
        pinyin_get_pinyin_key_rest (m_instance, i, &pos);

        guint16 cursor = 0, end = 0;
        pinyin_get_pinyin_key_rest_positions (m_instance, pos, &cursor, &end);

        gchar * str = NULL;
        if (G_UNLIKELY (cursor == m_cursor)) { /* at word boundary. */
            pinyin_get_pinyin_string (m_instance, key, &str);
            m_buffer << '|' << str;
            g_free (str);
        } else if (G_LIKELY (cursor < m_cursor &&
                             m_cursor < end)) { /* in word */
            guint16 length = 0;
            pinyin_get_pinyin_key_rest_length (m_instance, pos, &length);

            /* raw text */
            String raw = m_text.substr (cursor, length);
            guint offset = m_cursor - cursor;
            m_buffer << ' ' << raw.substr (0, offset)
                     << '|' << raw.substr (offset);
        } else { /* other words */
            pinyin_get_pinyin_string (m_instance, key, &str);
            m_buffer << ' ' << str;
            g_free (str);
        }
    }

    if (m_cursor == m_pinyin_len)
        m_buffer << '|';

    /* append rest text */
    const gchar * p = m_text.c_str() + m_pinyin_len;
    m_buffer << p;

    StaticText aux_text (m_buffer);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}
