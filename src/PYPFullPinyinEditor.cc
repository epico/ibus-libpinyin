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

#include "PYPFullPinyinEditor.h"
#include "PYConfig.h"
#include "PYLibPinyin.h"

using namespace PY;

LibPinyinFullPinyinEditor::LibPinyinFullPinyinEditor
(PinyinProperties & props, Config & config)
    : LibPinyinPinyinEditor (props, config)
{
    m_instance = LibPinyinBackEnd::instance ().allocPinyinInstance ();
}

LibPinyinFullPinyinEditor::~LibPinyinFullPinyinEditor (void)
{
    LibPinyinBackEnd::instance ().freePinyinInstance (m_instance);
    m_instance = NULL;
}

void
LibPinyinFullPinyinEditor::reset (void)
{
    LibPinyinPinyinEditor::reset ();
}

gboolean
LibPinyinFullPinyinEditor::insert (gint ch)
{
    /* is full */
    if (G_UNLIKELY (m_text.length () >= MAX_PINYIN_LEN))
        return TRUE;

    m_text.insert (m_cursor++, ch);

    updatePinyin ();
    update ();
    return TRUE;
}


gboolean
LibPinyinFullPinyinEditor::processKeyEvent (guint keyval,
                                            guint keycode,
                                            guint modifiers)
{
    return LibPinyinPinyinEditor::processKeyEvent (keyval, keycode, modifiers);
}

void
LibPinyinFullPinyinEditor::updatePinyin (void)
{
    if (G_UNLIKELY (m_text.empty ())) {
        m_pinyin_len = 0;
        /* TODO: check whether to replace "" with NULL. */
        pinyin_parse_more_full_pinyins (m_instance, "");
        pinyin_guess_sentence(m_instance);
        return;
    }

    m_pinyin_len =
        pinyin_parse_more_full_pinyins (m_instance, m_text.c_str ());
    pinyin_guess_sentence (m_instance);
}

void
LibPinyinFullPinyinEditor::updateAuxiliaryText ()
{
    if (G_UNLIKELY (m_text.empty ())) {
        hideAuxiliaryText ();
        return;
    }

    m_buffer.clear ();

    // guint pinyin_cursor = getPinyinCursor ();
    PinyinKeyVector & pinyin_keys = m_instance->m_pinyin_keys;
    PinyinKeyPosVector & pinyin_poses = m_instance->m_pinyin_key_rests;
    for (guint i = 0; i < pinyin_keys->len; ++i) {
        PinyinKey *key = &g_array_index (pinyin_keys, PinyinKey, i);
        PinyinKeyPos *pos = &g_array_index (pinyin_poses, PinyinKeyPos, i);
        guint cursor = pos->m_raw_begin;

        if (G_UNLIKELY (cursor == m_cursor)) { /* at word boundary. */
            m_buffer << '|' << key->get_pinyin_string ();
        } else if (G_LIKELY ( cursor < m_cursor &&
                              m_cursor < pos->m_raw_end )) { /* in word */
            /* raw text */
            String raw = m_text.substr (cursor,
                                        pos->length ());
            guint offset = m_cursor - cursor;
            m_buffer << ' ' << raw.substr (0, offset)
                     << '|' << raw.substr (offset);
        } else { /* other words */
            m_buffer << ' ' << key->get_pinyin_string ();
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

void
LibPinyinFullPinyinEditor::update (void)
{
    guint lookup_cursor = getLookupCursor ();
    pinyin_get_full_pinyin_candidates (m_instance, lookup_cursor, m_candidates);

    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}
