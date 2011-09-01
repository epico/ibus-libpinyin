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
#include "PYLibPinyinBaseEditor.h"
#include "PYConfig.h"
#include "PYPinyinProperties.h"
#include "PYSimpTradConverter.h"

namespace PY {
/* init static members */
LibPinyinBaseEditor::LibPinyinBaseEditor (PinyinProperties &props,
                                          Config &config):
    Editor (props, config),
    m_pinyin (MAX_PHRASE_LEN),
    m_pinyin_len (0),
    m_lookup_table (m_config.pageSize ())
{
}

gboolean
LibPinyinBaseEditor::processSpace (guint keyval, guint keycode,
                                   guint modifiers)
{
    if (!m_text)
        return FALSE;
    if (cmshm_filter (modifiers) != 0)
        return TRUE;
    if (m_lookup_table.size () != 0) {
        selectCandidate (m_lookup_table.cursorPos ());
    } else {
        commit ();
    }
    return TRUE;
}

gboolean
LibPinyinBaseEditor::processFunctionKey (guint keyval, guint keycode, guint modifiers)
{
    if (m_text.empty ())
        return FALSE;

    /* ignore numlock */
    modifiers = cmshm_filter (modifiers);

    if (modifiers != 0 && modifiers != IBUS_CONTROL_MASK)
        return TRUE;

    /* process some cursor control keys */
    if (modifiers == 0) {  /* no modifiers. */
        switch (keyval) {
        case IBUS_Return:
        case IBUS_KP_Enter:
            commit ();
            return TRUE;

        case IBUS_BackSpace:
            removeCharBefore ();
            return TRUE;

        case IBUS_Delete:
        case IBUS_KP_Delete:
            removeCharAfter ();
            return TRUE;

        case IBUS_Left:
        case IBUS_KP_Left:
            moveCursorLeft ();
            return TRUE;

        case IBUS_Right:
        case IBUS_KP_Right:
            moveCursorRight ();
            return TRUE;

        case IBUS_Home:
        case IBUS_KP_Home:
            moveCursorToBegin ();
            return TRUE;

        case IBUS_End:
        case IBUS_KP_End:
            moveCursorToEnd ();
            return TRUE;

        case IBUS_Up:
        case IBUS_KP_Up:
            cursorUp ();
            return TRUE;

        case IBUS_Down:
        case IBUS_KP_Down:
            cursorDown ();
            return TRUE;

        case IBUS_Page_Up:
        case IBUS_KP_Page_Up:
            pageUp ();
            return TRUE;

        case IBUS_Page_Down:
        case IBUS_KP_Page_Down:
        case IBUS_Tab:
            pageDown ();
            return TRUE;

        case IBUS_Escape:
            reset ();
            return TRUE;
        default:
            return TRUE;
        }
    } else { /* ctrl key pressed. */
        switch (keyval) {
        case IBUS_BackSpace:
            removeWordBefore ();
            return TRUE;

        case IBUS_Delete:
        case IBUS_KP_Delete:
            removeWordAfter ();
            return TRUE;

        case IBUS_Left:
        case IBUS_KP_Left:
            moveCursorLeftByWord ();
            return TRUE;

        case IBUS_Right:
        case IBUS_KP_Right:
            moveCursorToEnd ();
            return TRUE;

        default:
            return TRUE;
        }
    }
    return TRUE;
}

gboolean
LibPinyinBaseEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
    return FALSE;
}

gboolean
LibPinyinBaseEditor::updateSpecialPhrases (void)
{
    m_special_phrases.clear ();

    if (!m_config.specialPhrases ())
        return FALSE;

    if (!m_selected_special_phrase.empty ())
        return FALSE;

    /* TODO: change behavior to match the entire m_text,
     *         instead of partial of m_text.
     */
    g_assert(FALSE);
}

void
LibPinyinBaseEditor::updateLookupTableFast (void)
{
    Editor::updateLookupTableFast (m_lookup_table, TRUE);
}

void
LibPinyinBaseEditor::updateLookupTable (void)
{
    m_lookup_table.clear ();

    fillLookupTableByPage ();
    if (m_lookup_table.size()) {
        Editor::updateLookupTable (m_lookup_table, TRUE);
    } else {
        hideLookupTable ();
    }
}

gboolean
LibPinyinBaseEditor::fillLookupTableByPage (void)
{
    if (!m_selected_special_phrase.empty ()) {
        return FALSE;
    }

    guint filled_nr = m_lookup_table.size ();
    guint page_size = m_lookup_table.pageSize ();

    /* fill lookup table by libpinyin get candidates. */
    g_assert(FALSE);
}

void
LibPinyinBaseEditor::pageUp (void)
{
    if (G_LIKELY (m_lookup_table.pageUp ())) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
LibPinyinBaseEditor::pageDown (void)
{
    if (G_LIKELY((m_lookup_table.pageDown ()) ||
                 (fillLookupTableByPage () && m_lookup_table.pageDown()))) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
LibPinyinBaseEditor::cursorUp (void)
{
    if (G_LIKELY (m_lookup_table.cursorUp ())) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
LibPinyinBaseEditor::cursorDown (void)
{
    if (G_LIKELY ((m_lookup_table.cursorPos () == m_lookup_table.size() - 1) &&
                  (fillLookupTableByPage () == FALSE))) {
        return;
    }

    if (G_LIKELY (m_lookup_table.cursorDown ())) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
LibPinyinBaseEditor::candidateClicked (guint index, guint button, guint state)
{
    selectCandidateInPage (index);
}

void
LibPinyinBaseEditor::reset (void)
{
    m_pinyin.clear ();
    m_pinyin_len = 0;
    m_lookup_table.clear ();
    m_special_phrases.clear ();
    m_selected_special_phrase.clear ();

    Editor::reset ();
}

void
LibPinyinBaseEditor::update (void)
{
    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}

void
LibPinyinBaseEditor::commit (const gchar *str)
{
    StaticText text(str);
    commitText (text);
}

gboolean
LibPinyinBaseEditor::selectCandidate (guint i)
{
    g_assert(FALSE);
    if (i < m_special_phrases.size ()) {
        /* select a special phrase */
        m_selected_special_phrase = m_special_phrases[i];

        /* refer to updateSpecialPhrases. */
    }
}

gboolean
LibPinyinBaseEditor::selectCandidateInPage (guint i)
{
    guint page_size = m_lookup_table.pageSize ();
    guint cursor_pos = m_lookup_table.cursorPos ();

    if (G_UNLIKELY (i >= page_size))
        return FALSE;
    i += (cursor_pos / page_size) * page_size;

    return selectCandidate (i);
}

};
