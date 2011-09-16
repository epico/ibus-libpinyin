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
#include "PYPBopomofoEditor.h"
#include "PYConfig.h"
#include "PYPinyinProperties.h"
#include "PYSimpTradConverter.h"

namespace PY {
#include "PYBopomofoKeyboard.h"
};

using namespace PY;

const static gchar * bopomofo_select_keys[] = {
    "1234567890",
    "asdfghjkl;",
    "1qaz2wsxed",
    "asdfzxcvgb",
    "1234qweras",
    "aoeu;qjkix",
    "aoeuhtnsid",
    "aoeuidhtns",
    "qweasdzxcr"
};

LibPinyinBopomofoEditor::LibPinyinBopomofoEditor
(PinyinProperties & props, Config & config)
    : LibPinyinPhoneticEditor (props, config),
      m_select_mode (FALSE)
{
}

LibPinyinBopomofoEditor::~LibPinyinBopomofoEditor (void)
{
}

void
LibPinyinBopomofoEditor::reset (void)
{
    m_select_mode = FALSE;
    LibPinyinPhoneticEditor::reset ();
}

gboolean
LibPinyinBopomofoEditor::insert (gint ch)
{
    /* is full */
    if (G_UNLIKELY (m_text.length () >= MAX_PINYIN_LEN))
        return TRUE;

    m_text.insert (m_cursor++, ch);
    update ();

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::removeCharBefore (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    m_cursor --;
    m_text.erase (m_cursor, 1);
    update();

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::removeCharAfter (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_text.erase (m_cursor, 1);
    update ();

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::removeWordBefore (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    /* TODO: to be implemented. */
    g_assert(FALSE);

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::removeWordAfter (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    /* TODO: remove one word instead of the sentence. */
    g_assert(FALSE);

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::moveCursorLeft (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    m_cursor --;
    update ();

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::moveCursorRight (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_cursor ++;
    update ();

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::moveCursorLeftByWord (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    /* TODO: to be implemented. */
    g_assert(FALSE);

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::moveCursorRightByWord (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    /* TODO: to be implemented. */
    g_assert(FALSE);

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::moveCursorToBegin (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    m_cursor = 0;
    update();

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::moveCursorToEnd (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_cursor = m_text.length ();
    update();
    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::processGuideKey (guint keyval, guint keycode,
                                          guint modifiers)
{
    if (!m_config.guideKey ())
        return FALSE;

    if (G_UNLIKELY (cmshm_filter (modifiers) != 0))
        return FALSE;

    if (G_LIKELY (m_select_mode))
        return FALSE;

    if (G_UNLIKELY (keyval == IBUS_space)) {
        m_select_mode = TRUE;
        update ();
        return TRUE;
    }

    return FALSE;
}

gboolean
LibPinyinBopomofoEditor::processAuxiliarySelectKey
(guint keyval, guint keycode, guint modifiers)
{
    if (G_UNLIKELY (cmshm_filter (modifiers) != 0))
        return FALSE;

    guint i;

    switch (keyval) {
    case IBUS_KP_0:
        i = 9;
        if (!m_config.auxiliarySelectKeyKP ())
            return FALSE;
        break;
    case IBUS_KP_1 ... IBUS_KP_9:
        i = keyval - IBUS_KP_1;
        if (!m_config.auxiliarySelectKeyKP ())
            return FALSE;
        break;
    case IBUS_F1 ... IBUS_F10:
        i = keyval - IBUS_F1;
        if (!m_config.auxiliarySelectKeyF ())
            return FALSE;
        break;
    default:
        return FALSE;
    }

    m_select_mode = TRUE;
    selectCandidateInPage (i);

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::processSelectKey (guint keyval, guint keycode,
                                           guint modifiers)
{
    if (G_UNLIKELY (!m_text))
        return FALSE;

    if (G_LIKELY (!m_select_mode && ((modifiers & IBUS_MOD1_MASK) == 0)))
        return FALSE;

    const gchar * pos = NULL;
    const gchar * keys = bopomofo_select_keys[m_config.selectKeys ()];
    for ( const gchar * p = keys; *p; ++p ) {
        if ( *p == keyval )
            pos = p;
    }

    if (pos == NULL)
        return FALSE;

    m_select_mode = TRUE;

    guint i = pos - keys;
    selectCandidateInPage (i);

    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::processBopomofo (guint keyval, guint keycode,
                                          guint modifiers)
{
    if (G_UNLIKELY (cmshm_filter (modifiers) != 0))
        return m_text ? TRUE : FALSE;

    if (keyvalToBopomofo (keyval) == BOPOMOFO_ZERO)
        return FALSE;

    m_select_mode = FALSE;

    return insert (keyval);
}

gboolean
LibPinyinBopomofoEditor::processKeyEvent (guint keyval, guint keycode,
                                          guint modifiers)
{
    modifiers &= (IBUS_SHIFT_MASK |
                  IBUS_CONTROL_MASK |
                  IBUS_MOD1_MASK |
                  IBUS_SUPER_MASK |
                  IBUS_HYPER_MASK |
                  IBUS_META_MASK |
                  IBUS_LOCK_MASK);

    if (G_UNLIKELY (processGuideKey (keyval, keycode, modifiers)))
        return TRUE;
    if (G_UNLIKELY (processSelectKey (keyval, keycode, modifiers)))
        return TRUE;
    if (G_UNLIKELY (processAuxiliarySelectKey (keyval, keycode,
                                               modifiers)))
        return TRUE;
    if (G_LIKELY (processBopomofo (keyval, keycode, modifiers)))
        return TRUE;

    switch (keyval) {
    case IBUS_space:
        m_select_mode = TRUE;
        return processSpace (keyval, keycode, modifiers);

    case IBUS_Up:        case IBUS_KP_Up:
    case IBUS_Down:      case IBUS_KP_Down:
    case IBUS_Page_Up:   case IBUS_KP_Page_Up:
    case IBUS_Page_Down: case IBUS_KP_Page_Down:
    case IBUS_Tab:
        m_select_mode = TRUE;
        return LibPinyinPhoneticEditor::processFunctionKey
            (keyval, keycode, modifiers);

    case IBUS_BackSpace:
    case IBUS_Delete:    case IBUS_KP_Delete:
    case IBUS_Left:      case IBUS_KP_Left:
    case IBUS_Right:     case IBUS_KP_Right:
    case IBUS_Home:      case IBUS_KP_Home:
    case IBUS_End:       case IBUS_KP_End:
        m_select_mode = FALSE;
        return LibPinyinPhoneticEditor::processFunctionKey
            (keyval, keycode, modifiers);

    default:
        return LibPinyinPhoneticEditor::processFunctionKey
            (keyval, keycode, modifiers);
    }
    return FALSE;
}

gint
LibPinyinBopomofoEditor::keyvalToBopomofo(gint ch)
{
    const gint keyboard = m_config.bopomofoKeyboardMapping ();    
    gint len = G_N_ELEMENTS (bopomofo_keyboard[keyboard]);

    for ( gint i = 0; i < len; ++i ) {
        if ( bopomofo_keyboard[keyboard][i][0] == ch )
            return bopomofo_keyboard[keyboard][i][1];
    }

    return BOPOMOFO_ZERO;
}
