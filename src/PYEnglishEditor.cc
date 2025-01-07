/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2010-2011 Peng Wu <alexepico@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PYEnglishEditor.h"
#include <string.h>
#include <string>
#include <stdio.h>
#include <limits>
#include <libintl.h>
#include "PYConfig.h"

#define _(text) (gettext(text))

namespace PY {

EnglishEditor::EnglishEditor (PinyinProperties & props, Config &config)
    : Editor (props, config), m_train_factor (0.1)
{
    m_english_database = & EnglishDatabase::instance ();
}

EnglishEditor::~EnglishEditor ()
{
    m_english_database = NULL;
}

gboolean
EnglishEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
    if (modifiers & IBUS_MOD4_MASK)
        return FALSE;

    //IBUS_SHIFT_MASK is removed.
    modifiers &= (IBUS_CONTROL_MASK |
                  IBUS_MOD1_MASK |
                  IBUS_SUPER_MASK |
                  IBUS_HYPER_MASK |
                  IBUS_META_MASK |
                  IBUS_LOCK_MASK);
    if (modifiers)
        return FALSE;

    //handle backspace/delete here.
    if (processEditKey (keyval))
        return TRUE;

    //handle page/cursor up/down here.
    if (processPageKey (keyval))
        return TRUE;

    //handle label key select here.
    if (processLabelKey (keyval))
        return TRUE;

    if (processSpace (keyval))
        return TRUE;

    if (processEnter (keyval))
        return TRUE;

    m_cursor = std::min (m_cursor, (guint)m_text.length ());

    /* Remember the input string. */
    if (m_cursor == 0) {
        g_return_val_if_fail ('v' == keyval || 'V' == keyval, FALSE);
        m_text.insert (m_cursor, keyval);
        m_cursor ++;
    } else {
        g_return_val_if_fail ('v' == m_text[0] || 'V' == m_text[0], FALSE);

        if ((keyval >= 'a' && keyval <= 'z') ||
            (keyval >= 'A' && keyval <= 'Z')) {
            m_text.insert (m_cursor, keyval);
            m_cursor ++;
        }

        if (keyval <= std::numeric_limits<char>::max() &&
            g_unichar_ispunct (keyval) &&
            EnglishSymbols.find(keyval) != std::string::npos) {
            m_text.insert (m_cursor, keyval);
            m_cursor ++;
        }

        if (!m_config.squareBracketPage () &&
            (IBUS_bracketleft == keyval || IBUS_bracketright == keyval)) {
            m_text.insert (m_cursor, keyval);
            m_cursor ++;
        }
    }

    /* Deal other staff with updateStateFromInput (). */
    updateStateFromInput ();
    update ();
    return TRUE;
}

gboolean
EnglishEditor::processEditKey (guint keyval)
{
    switch (keyval) {
    case IBUS_Delete:
    case IBUS_KP_Delete:
        removeCharAfter ();
        updateStateFromInput ();
        update ();
        return TRUE;
    case IBUS_BackSpace:
        removeCharBefore ();
        updateStateFromInput ();
        update ();
        return TRUE;
    }
    return FALSE;
}

gboolean
EnglishEditor::processPageKey (guint keyval)
{
    switch (keyval) {
    case IBUS_comma:
        if (m_config.commaPeriodPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
    case IBUS_minus:
        if (m_config.minusEqualPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
    case IBUS_bracketleft:
        if (m_config.squareBracketPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
    case IBUS_period:
        if (m_config.commaPeriodPage ()) {
            pageDown ();
            return TRUE;
        }
        break;
    case IBUS_equal:
        if (m_config.minusEqualPage ()) {
            pageDown ();
            return TRUE;
        }
        break;
    case IBUS_bracketright:
        if (m_config.squareBracketPage ()) {
            pageDown ();
            return TRUE;
        }
        break;

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
        pageDown ();
        return TRUE;

    case IBUS_Escape:
        reset ();
        return TRUE;
    }
    return FALSE;
}

gboolean
EnglishEditor::processLabelKey (guint keyval)
{
    switch (keyval) {
    case '1' ... '9':
        return selectCandidateInPage (keyval - '1');
        break;
    case '0':
        return selectCandidateInPage (9);
        break;
    }
    return FALSE;
}

gboolean
EnglishEditor::processEnter (guint keyval)
{
    if (keyval != IBUS_Return)
        return FALSE;

    if (m_text.length () == 0)
        return FALSE;

    String word = m_text;
    word.erase (0, 1);

    Text text(word);
    commitText (text);
    m_english_database->train (word.c_str (), m_train_factor);
    reset ();
    return TRUE;
}

gboolean
EnglishEditor::processSpace (guint keyval)
{
    if (!(keyval == IBUS_space || keyval == IBUS_KP_Space))
        return FALSE;

    if (m_text == "v" || m_text == "V") {
        reset ();
        return TRUE;
    }

    guint cursor_pos = m_lookup_table.cursorPos ();
    return selectCandidate (cursor_pos);
}

void
EnglishEditor::candidateClicked (guint index, guint button, guint state)
{
    selectCandidateInPage (index);
}

gboolean
EnglishEditor::selectCandidateInPage (guint index)
{
    guint page_size = m_lookup_table.pageSize ();
    guint cursor_pos = m_lookup_table.cursorPos ();

    if (G_UNLIKELY (index >= page_size))
        return FALSE;
    index += (cursor_pos / page_size) * page_size;

    return selectCandidate (index);
}

gboolean
EnglishEditor::selectCandidate (guint index)
{
    if (index >= m_lookup_table.size ())
        return FALSE;

    IBusText *candidate = m_lookup_table.getCandidate (index);
    Text text (candidate);
    commitText (text);
    m_english_database->train (candidate->text, m_train_factor);
    reset ();
    return TRUE;
}

gboolean
EnglishEditor::updateStateFromInput (void)
{
    /* Do parse and candidates update here. */
    /* prefix v double check here. */
    if (m_text.empty ()) {
        m_preedit_text = "";
        m_auxiliary_text = "";
        m_cursor = 0;
        clearLookupTable ();
        return FALSE;
    }

    if ('v' != m_text[0] && 'V' != m_text[0]) {
        g_warning ("v is expected in m_text string.\n");
        m_auxiliary_text = "";
        clearLookupTable ();
        return FALSE;
    }

    m_auxiliary_text = m_text[0];

    if (1 == m_text.length ()) {
        clearLookupTable ();

        const char * help_string = _("Please input the English word.");
        int space_len = std::max ( 0, m_aux_text_len
                                   - (int) g_utf8_strlen (help_string, -1));
        m_auxiliary_text.append (space_len, ' ');
        m_auxiliary_text += help_string;

        return TRUE;
    }

    m_auxiliary_text += " ";

    String prefix = m_text.substr (1);
    m_auxiliary_text += prefix;

    /* lookup table candidate fill here. */
    std::vector<std::string> words;
    gboolean retval = m_english_database->listWords (prefix.c_str (), words);
    if (!retval)
        return FALSE;

    clearLookupTable ();
    std::vector<std::string>::iterator iter;
    for (iter = words.begin (); iter != words.end (); ++iter){
        Text text (*iter);
        m_lookup_table.appendCandidate (text);
    }
    return TRUE;
}

/* Auxiliary Functions */

void
EnglishEditor::pageUp (void)
{
    if (G_LIKELY (m_lookup_table.pageUp ())) {
        update ();
    }
}

void
EnglishEditor::pageDown (void)
{
    if (G_LIKELY (m_lookup_table.pageDown ())) {
        update ();
    }
}

void
EnglishEditor::cursorUp (void)
{
    if (G_LIKELY (m_lookup_table.cursorUp ())) {
        update ();
    }
}

void
EnglishEditor::cursorDown (void)
{
    if (G_LIKELY (m_lookup_table.cursorDown ())) {
        update ();
    }
}

void
EnglishEditor::update (void)
{
    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}

void
EnglishEditor::updateAll (void)
{
    updateStateFromInput ();
    update ();
}

void
EnglishEditor::reset (void)
{
    m_text = "";
    updateStateFromInput ();
    update ();
}

void
EnglishEditor::clearLookupTable (void)
{
    m_lookup_table.clear ();
    m_lookup_table.setPageSize (m_config.pageSize ());
    m_lookup_table.setOrientation (m_config.orientation ());
}

void
EnglishEditor::updateLookupTable (void)
{
    if (m_lookup_table.size ()) {
        Editor::updateLookupTableFast (m_lookup_table, TRUE);
    } else {
        hideLookupTable ();
    }
}

void
EnglishEditor::updatePreeditText (void)
{
    if (G_UNLIKELY (m_preedit_text.empty ())) {
        hidePreeditText ();
        return;
    }

    StaticText preedit_text (m_preedit_text);
    Editor::updatePreeditText (preedit_text, m_cursor, TRUE);
}

void
EnglishEditor::updateAuxiliaryText (void)
{
    if (G_UNLIKELY (m_auxiliary_text.empty ())) {
        hideAuxiliaryText ();
        return;
    }
    
    StaticText aux_text (m_auxiliary_text);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}

gboolean
EnglishEditor::removeCharBefore (void)
{
    if (G_UNLIKELY (m_cursor <= 0)) {
        m_cursor = 0;
        return FALSE;
    }

    if (G_UNLIKELY (m_cursor > m_text.length ())) {
        m_cursor = m_text.length ();
        return FALSE;
    }

    m_text.erase (m_cursor - 1, 1);
    m_cursor = std::max (0, static_cast<int>(m_cursor) - 1);
    return TRUE;
}

gboolean
EnglishEditor::removeCharAfter (void)
{
    if (G_UNLIKELY (m_cursor < 0)) {
        m_cursor = 0;
        return FALSE;
    }

    if (G_UNLIKELY (m_cursor >= m_text.length ())) {
        m_cursor = m_text.length ();
        return FALSE;
    }

    m_text.erase (m_cursor, 1);
    m_cursor = std::min (m_cursor, (guint) m_text.length ());
    return TRUE;
}

};
