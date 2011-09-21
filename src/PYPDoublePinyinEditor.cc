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

#include "PYPDoublePinyinEditor.h"

using namespace PY;


gboolean
LibPinyinDoublePinyinEditor::processKeyEvent (guint keyval, guint keycode,
                                              guint modifiers)
{
    /* handle ';' key */
    if (G_UNLIKELY (keyval == IBUS_semicolon)) {
        if (cmshm_filter (modifiers) == 0) {
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
        m_pinyins.clear ();
        m_pinyin_len = 0;
        /* TODO: check whether to replace "" with NULL. */
        pinyin_parse_more_double_pinyins (m_instance, "");
        return;
    }

    m_pinyin_len =
        pinyin_parse_more_double_pinyins (m_instance, m_text.c_str ());
    pinyin_guess_sentence (m_instance);
}
