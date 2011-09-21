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
LibPinyinFullPinyinEditor::removeCharBefore (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    m_cursor --;
    m_text.erase (m_cursor, 1);

    updatePinyin ();
    update ();

    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::removeCharAfter (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_text.erase (m_cursor, 1);

    updatePinyin ();
    update ();

    return TRUE;
}

guint
LibPinyinFullPinyinEditor::getCursorLeftByWord (void)
{
    guint cursor;

    if (G_UNLIKELY (m_cursor > m_pinyin_len)) {
        cursor = m_pinyin_len;
    } else {
        PinyinKeyPosVector & pinyin_poses = m_instance->m_pinyin_poses;
        guint pinyin_cursor = getPinyinCursor ();
        PinyinKeyPos *pos = &g_array_index
            (pinyin_poses, PinyinKeyPos, pinyin_cursor);
        cursor = pos->m_pos;

        /* cursor at the begin of one pinyin */
        g_return_val_if_fail (pinyin_cursor > 0, 0);
        if ( cursor == m_cursor) {
            pos = &g_array_index
                (pinyin_poses, PinyinKeyPos, pinyin_cursor - 1);
            cursor = pos->m_pos;
        }
    }

    return cursor;
}

guint
LibPinyinFullPinyinEditor::getCursorRightByWord (void)
{
    guint cursor;

    if (G_UNLIKELY (m_cursor > m_pinyin_len)) {
        cursor = m_text.length ();
    } else {
        guint pinyin_cursor = getPinyinCursor ();
        PinyinKeyPos *pos = &g_array_index
            (m_instance->m_pinyin_poses, PinyinKeyPos, pinyin_cursor);
        cursor = pos->get_end_pos ();
    }

    return cursor;
}

gboolean
LibPinyinFullPinyinEditor::removeWordBefore (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    guint cursor = getCursorLeftByWord ();
    m_text.erase (cursor, m_cursor - cursor);
    m_cursor = cursor;
    updatePinyin ();
    update ();
    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::removeWordAfter (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    guint cursor = getCursorRightByWord ();
    m_text.erase (m_cursor, cursor - m_cursor);
    updatePinyin ();
    update ();
    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::moveCursorLeft (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    m_cursor --;
    update ();
    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::moveCursorRight (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_cursor ++;
    update ();
    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::moveCursorLeftByWord (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    guint cursor = getCursorLeftByWord ();

    m_cursor = cursor;
    update ();
    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::moveCursorRightByWord (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    guint cursor = getCursorRightByWord ();

    m_cursor = cursor;
    update ();
    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::moveCursorToBegin (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return TRUE;

    m_cursor = 0;
    update ();
    return TRUE;
}

gboolean
LibPinyinFullPinyinEditor::moveCursorToEnd (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_cursor = m_text.length ();
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
