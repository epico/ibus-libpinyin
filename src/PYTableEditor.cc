/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2012 Peng Wu <alexepico@gmail.com>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "PYTableEditor.h"
#include <string.h>
#include <string>
#include <vector>
#include <libintl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "PYString.h"
#include "PYConfig.h"

#define _(text) (gettext (text))

#define TABLE_DATABASE_ADD_FREQUENCY 10

namespace PY {

TableEditor::TableEditor (PinyinProperties &props, Config &config)
    : Editor (props, config)
{
}

TableEditor::~TableEditor ()
{
}

gboolean
TableEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
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
        g_return_val_if_fail ('u' == keyval || 'U' == keyval, FALSE);
        m_text.insert (m_cursor, keyval);
        m_cursor ++;
    } else {
        g_return_val_if_fail ('u' == m_text[0] || 'U' == m_text[0], FALSE);
        if (keyval >= 'a' && keyval <= 'z') {
            /* only lower case characters here */
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
TableEditor::processEditKey (guint keyval)
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
TableEditor::processPageKey (guint keyval)
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
TableEditor::processLabelKey (guint keyval)
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
TableEditor::processEnter (guint keyval)
{
    if (keyval != IBUS_Return)
        return FALSE;

    if (m_text.length () == 0)
        return FALSE;

    Text text(m_text);
    commitText (text);
    reset ();
    return TRUE;
}

gboolean
TableEditor::processSpace (guint keyval)
{
    if (!(keyval == IBUS_space || keyval == IBUS_KP_Space))
        return FALSE;

    guint cursor_pos = m_lookup_table.cursorPos ();
    return selectCandidate (cursor_pos);
}

void
TableEditor::candidateClicked (guint index, guint button, guint state)
{
    selectCandidateInPage (index);
}

gboolean
TableEditor::selectCandidateInPage (guint index)
{
    guint page_size = m_lookup_table.pageSize ();
    guint cursor_pos = m_lookup_table.cursorPos ();

    if (G_UNLIKELY (index >= page_size))
        return FALSE;
    index += (cursor_pos / page_size) * page_size;

    return selectCandidate (index);
}

gboolean
TableEditor::selectCandidate (guint index)
{
    if (index >= m_lookup_table.size ())
        return FALSE;

    IBusText *candidate = m_lookup_table.getCandidate (index);
    Text text (candidate);

    if (m_config.useCustomTable ()) {
        TableDatabase *table_database = &TableDatabase::userInstance ();
        int freq = 0;
        table_database->getPhraseInfo (text.text (), freq);
        freq += TABLE_DATABASE_ADD_FREQUENCY;
        table_database->updatePhrase (text.text (), freq);
    }

    commitText (text);
    reset ();
    return TRUE;
}

TableDatabase *
TableEditor::getTableDatabase (void)
{
    if (!m_config.useCustomTable ())
        return &TableDatabase::systemInstance ();
    else
        return &TableDatabase::userInstance ();
    return NULL;
}

gboolean
TableEditor::updateStateFromInput (void)
{
    /* Do parse and candidates update here. */
    /* prefix u double check here. */
    if (m_text.empty ()) {
        m_preedit_text = "";
        m_auxiliary_text = "";
        m_cursor = 0;
        clearLookupTable ();
        return FALSE;
    }

    if ('u' != m_text[0] && 'U' != m_text[0]) {
        g_warning ("u is expected in m_text string.\n");
        m_auxiliary_text = "";
        clearLookupTable ();
        return FALSE;
    }

    m_auxiliary_text = m_text[0];

    if (1 == m_text.length ()) {
        clearLookupTable ();

        const char * help_string =
            _("Please use \"hspnz\" to input.");
        if (m_config.useCustomTable ())
            help_string =
                _("Please use table code to input.");
        int space_len = std::max ( 0, m_aux_text_len
                                   - (int) g_utf8_strlen (help_string, -1));
        m_auxiliary_text.append(space_len, ' ');
        m_auxiliary_text += help_string;

        return TRUE;
    }

    m_auxiliary_text += " ";

    String prefix = m_text.substr (1);
    m_auxiliary_text += prefix;

    /* lookup table candidate fill here. */
    TableDatabase *table_database = getTableDatabase ();
    std::vector<std::string> characters;
    gboolean retval = table_database->listPhrases
        (prefix.c_str (), characters);
    if (!retval)
        return FALSE;

    clearLookupTable ();
    std::vector<std::string>::iterator iter;
    for (iter = characters.begin (); iter != characters.end (); ++iter){
        Text text(*iter);
        m_lookup_table.appendCandidate (text);
    }
    return TRUE;
}

/* Auxiliary Functions */

void
TableEditor::pageUp (void)
{
    if (G_LIKELY (m_lookup_table.pageUp ())) {
        update ();
    }
}

void
TableEditor::pageDown (void)
{
    if (G_LIKELY (m_lookup_table.pageDown ())) {
        update ();
    }
}

void
TableEditor::cursorUp (void)
{
    if (G_LIKELY (m_lookup_table.cursorUp ())) {
        update ();
    }
}

void
TableEditor::cursorDown (void)
{
    if (G_LIKELY (m_lookup_table.cursorDown ())) {
        update ();
    }
}

void
TableEditor::update (void)
{
    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}

void
TableEditor::updateAll (void)
{
    updateStateFromInput ();
    update ();
}

void
TableEditor::reset (void)
{
    m_text = "";
    updateStateFromInput ();
    update ();
}

void
TableEditor::clearLookupTable (void)
{
    m_lookup_table.clear ();
    m_lookup_table.setPageSize (m_config.pageSize ());
    m_lookup_table.setOrientation (m_config.orientation ());
}

void
TableEditor::updateLookupTable (void)
{
    if (m_lookup_table.size ()){
        Editor::updateLookupTableFast (m_lookup_table, TRUE);
    } else {
        hideLookupTable ();
    }
}

void
TableEditor::updatePreeditText (void)
{
    if (G_UNLIKELY (m_preedit_text.empty ())) {
        hidePreeditText ();
        return;
    }

    StaticText preedit_text (m_preedit_text);
    Editor::updatePreeditText (preedit_text, m_cursor, TRUE);
}

void
TableEditor::updateAuxiliaryText (void)
{
    if (G_UNLIKELY (m_auxiliary_text.empty ())) {
        hideAuxiliaryText ();
        return;
    }

    StaticText aux_text (m_auxiliary_text);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}

gboolean
TableEditor::removeCharBefore (void)
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
TableEditor::removeCharAfter (void)
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
