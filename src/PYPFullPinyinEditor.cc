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

