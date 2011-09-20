/* vim:set et ts=4 sts=4:
 *
 * ibus-pinyin - The Chinese PinYin engine for IBus
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "PYPFullPinyinEditor.h"
#include "PYConfig.h"

using namespace PY;

LibPinyinFullPinyinEditor::LibPinyinFullPinyinEditor
(PinyinProperties & props, Config & config)
    : LibPinyinPinyinEditor (props, config)
{
}

LibPinyinFullPinyinEditor::~LibPinyinFullPinyinEditor (void)
{
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
        m_pinyins.clear ();
        m_pinyin_len = 0;
        return;
    }

    m_pinyin_len = PinyinParser::parse (m_text,               // text
                                        m_text.length (),     // text length
                                        m_config.option (),   // option
                                        m_pinyins,            // result
                                        MAX_PHRASE_LEN);      // max result length

    /* propagate to libpinyin */
    g_array_set_size (m_instance->m_pinyin_keys, 0);
    g_array_set_size (m_instance->m_pinyin_poses, 0);

    PinyinKey key; PinyinKeyPos pos;
    PinyinArray::const_iterator iter = m_pinyins.begin ();
    for ( ; iter != m_pinyins.end (); ++iter ) {
        PinyinSegment py = *iter;
        pinyin_parse_full_pinyin (m_instance, py.pinyin->text, &key);
        pos.set_pos (py.begin); pos.set_length (py.len);
        g_array_append_val(m_instance->m_pinyin_keys, key);
        g_array_append_val(m_instance->m_pinyin_poses, pos);
    }

    pinyin_guess_sentence(m_instance);
}
