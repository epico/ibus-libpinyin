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
#include "PYPPhoneticEditor.h"
#include "PYConfig.h"
#include "PYPinyinProperties.h"
#include "PYSimpTradConverter.h"

using namespace PY;

/* init static members */
PhoneticEditor::PhoneticEditor (PinyinProperties &props,
                                                  Config &config):
    Editor (props, config),
    m_pinyin_len (0),
    m_lookup_table (m_config.pageSize ())
{
}

PhoneticEditor::~PhoneticEditor (){
}

gboolean
PhoneticEditor::processSpace (guint keyval, guint keycode,
                                       guint modifiers)
{
    if (!m_text)
        return FALSE;
    if (cmshm_filter (modifiers) != 0)
        return TRUE;

    if (m_lookup_table.size () != 0) {
        selectCandidate (m_lookup_table.cursorPos ());
        update ();
    }
    else {
        commit ();
    }

    return TRUE;
}

gboolean
PhoneticEditor::processFunctionKey (guint keyval, guint keycode, guint modifiers)
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
            commit (m_text.c_str ());
            reset ();
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
            moveCursorRightByWord ();
            return TRUE;

        default:
            return TRUE;
        }
    }
    return TRUE;
}

gboolean
PhoneticEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
    return FALSE;
}

void
PhoneticEditor::updateLookupTableFast (void)
{
    Editor::updateLookupTableFast (m_lookup_table, TRUE);
}

void
PhoneticEditor::updateLookupTable (void)
{
    m_lookup_table.clear ();

    fillLookupTable ();
    if (m_lookup_table.size()) {
        Editor::updateLookupTable (m_lookup_table, TRUE);
    } else {
        hideLookupTable ();
    }
}

#if 0
gboolean
PhoneticEditor::fillLookupTableByPage (void)
{
    guint len = 0;
    pinyin_get_n_candidate (m_instance, &len);

    guint filled_nr = m_lookup_table.size ();
    guint page_size = m_lookup_table.pageSize ();

    /* fill lookup table by libpinyin get candidates. */
    guint need_nr = MIN (page_size, len - filled_nr);
    g_assert (need_nr >=0);
    if (need_nr == 0)
        return FALSE;

    String word;
    for (guint i = filled_nr; i < filled_nr + need_nr; i++) {
        if (i >= len)  /* no more candidates */
            break;

        lookup_candidate_t * candidate = NULL;
        pinyin_get_candidate (m_instance, i, &candidate);

        const gchar * phrase_string = NULL;
        pinyin_get_candidate_string (m_instance, candidate, &phrase_string);

        /* show get candidates. */
        if (G_LIKELY (m_props.modeSimp ())) {
            word = phrase_string;
        } else { /* Traditional Chinese */
            word.truncate (0);
            SimpTradConverter::simpToTrad (phrase_string, word);
        }

        Text text (word);
        m_lookup_table.appendCandidate (text);
    }

    return TRUE;
}
#endif

gboolean
PhoneticEditor::fillLookupTable (void)
{
    guint len = 0;
    pinyin_get_n_candidate (m_instance, &len);

    String word;
    for (guint i = 0; i < len; i++) {
        lookup_candidate_t * candidate = NULL;
        pinyin_get_candidate (m_instance, i, &candidate);

        const gchar * phrase_string = NULL;
        pinyin_get_candidate_string (m_instance, candidate, &phrase_string);

        /* show get candidates. */
        if (G_LIKELY (m_props.modeSimp ())) {
            word = phrase_string;
        } else { /* Traditional Chinese */
            word.truncate (0);
            SimpTradConverter::simpToTrad (phrase_string, word);
        }

        Text text (word);
        m_lookup_table.appendCandidate (text);
    }

    return TRUE;
}

void
PhoneticEditor::pageUp (void)
{
    if (G_LIKELY (m_lookup_table.pageUp ())) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
PhoneticEditor::pageDown (void)
{
    if (G_LIKELY(m_lookup_table.pageDown ())) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
PhoneticEditor::cursorUp (void)
{
    if (G_LIKELY (m_lookup_table.cursorUp ())) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
PhoneticEditor::cursorDown (void)
{
    if (G_LIKELY (m_lookup_table.cursorDown ())) {
        updateLookupTableFast ();
        updatePreeditText ();
        updateAuxiliaryText ();
    }
}

void
PhoneticEditor::candidateClicked (guint index, guint button, guint state)
{
    selectCandidateInPage (index);
}

void
PhoneticEditor::reset (void)
{
    m_pinyin_len = 0;
    m_lookup_table.clear ();

    pinyin_reset (m_instance);

    Editor::reset ();
}

void
PhoneticEditor::update (void)
{
    guint lookup_cursor = getLookupCursor ();
    pinyin_guess_candidates (m_instance, lookup_cursor);

    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}

void
PhoneticEditor::commit (const gchar *str)
{
    StaticText text(str);
    commitText (text);
}

guint
PhoneticEditor::getPinyinCursor ()
{
    guint len = 0;

    /* Translate cursor position to pinyin position. */
    guint16 pinyin_cursor = 0;
    pinyin_get_pinyin_key_rest_offset (m_instance, m_cursor, &pinyin_cursor);

    return pinyin_cursor;
}

guint
PhoneticEditor::getLookupCursor (void)
{
    guint len = 0;
    pinyin_get_n_pinyin (m_instance, &len);
    guint lookup_cursor = getPinyinCursor ();

    /* show candidates when pinyin cursor is at end. */
    if (lookup_cursor == len)
        lookup_cursor = 0;
    return lookup_cursor;
}

gboolean
PhoneticEditor::selectCandidate (guint i)
{
    guint len = 0;
    pinyin_get_n_candidate (m_instance, &len);

    if (G_UNLIKELY (i >= len))
        return FALSE;

    guint lookup_cursor = getLookupCursor ();

    lookup_candidate_t * candidate = NULL;
    pinyin_get_candidate (m_instance, i, &candidate);

    lookup_candidate_type_t type;
    pinyin_get_candidate_type (m_instance, candidate, &type);

    if (BEST_MATCH_CANDIDATE == type) {
        commit ();
        return TRUE;
    }

    lookup_cursor = pinyin_choose_candidate
        (m_instance, lookup_cursor, candidate);

    if (DIVIDED_CANDIDATE == type ||
        RESPLIT_CANDIDATE == type) {
        const gchar * str = NULL;
        pinyin_get_raw_full_pinyin (m_instance, &str);

        m_text = str;
        updatePinyin ();
    }
    pinyin_guess_sentence (m_instance);

    len = 0;
    pinyin_get_n_pinyin (m_instance, &len);
    if (lookup_cursor == len) {
        pinyin_train(m_instance);
        commit();
        return TRUE;
    }

    PinyinKeyPos *pos = NULL;
    pinyin_get_pinyin_key_rest (m_instance, lookup_cursor, &pos);

    guint16 begin = 0;
    pinyin_get_pinyin_key_rest_positions (m_instance, pos, &begin, NULL);
    m_cursor = begin;

    return TRUE;
}

gboolean
PhoneticEditor::selectCandidateInPage (guint i)
{
    guint page_size = m_lookup_table.pageSize ();
    guint cursor_pos = m_lookup_table.cursorPos ();

    if (G_UNLIKELY (i >= page_size))
        return FALSE;
    i += (cursor_pos / page_size) * page_size;

    return selectCandidate (i);
}

gboolean
PhoneticEditor::removeCharBefore (void)
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
PhoneticEditor::removeCharAfter (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_text.erase (m_cursor, 1);

    updatePinyin ();
    update ();

    return TRUE;
}

gboolean
PhoneticEditor::moveCursorLeft (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    m_cursor --;
    update ();
    return TRUE;
}

gboolean
PhoneticEditor::moveCursorRight (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_cursor ++;
    update ();
    return TRUE;
}

gboolean
PhoneticEditor::moveCursorToBegin (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    m_cursor = 0;
    update ();
    return TRUE;
}

gboolean
PhoneticEditor::moveCursorToEnd (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    m_cursor = m_text.length ();
    update ();
    return TRUE;
}


/* move cursor by word functions */

guint
PhoneticEditor::getCursorLeftByWord (void)
{
    guint16 cursor = 0;

    if (G_UNLIKELY (m_cursor > m_pinyin_len)) {
        cursor = m_pinyin_len;
    } else {
        guint len = 0;
        pinyin_get_n_pinyin (m_instance, &len);

        guint pinyin_cursor = getPinyinCursor ();

        PinyinKeyPos *pos = NULL;

        if (pinyin_cursor < len) {
            pinyin_get_pinyin_key_rest (m_instance, pinyin_cursor, &pos);

            pinyin_get_pinyin_key_rest_positions
                (m_instance, pos, &cursor, NULL);
        } else {
            /* at the end of pinyin string. */
            cursor  = m_cursor;
        }

        /* cursor at the begin of one pinyin */
        g_return_val_if_fail (pinyin_cursor > 0, 0);
        if ( cursor == m_cursor) {
            pinyin_get_pinyin_key_rest (m_instance, pinyin_cursor - 1, &pos);

            pinyin_get_pinyin_key_rest_positions
                (m_instance, pos, &cursor, NULL);
        }
    }

    return cursor;
}

guint
PhoneticEditor::getCursorRightByWord (void)
{
    guint16 cursor = 0;

    if (G_UNLIKELY (m_cursor > m_pinyin_len)) {
        cursor = m_text.length ();
    } else {
        guint pinyin_cursor = getPinyinCursor ();
        PinyinKeyPos *pos = NULL;
        pinyin_get_pinyin_key_rest (m_instance, pinyin_cursor, &pos);
        pinyin_get_pinyin_key_rest_positions (m_instance, pos, NULL, &cursor);
    }

    return cursor;
}

gboolean
PhoneticEditor::removeWordBefore (void)
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
PhoneticEditor::removeWordAfter (void)
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
PhoneticEditor::moveCursorLeftByWord (void)
{
    if (G_UNLIKELY (m_cursor == 0))
        return FALSE;

    guint cursor = getCursorLeftByWord ();

    m_cursor = cursor;
    update ();
    return TRUE;
}

gboolean
PhoneticEditor::moveCursorRightByWord (void)
{
    if (G_UNLIKELY (m_cursor == m_text.length ()))
        return FALSE;

    guint cursor = getCursorRightByWord ();

    m_cursor = cursor;
    update ();
    return TRUE;
}
