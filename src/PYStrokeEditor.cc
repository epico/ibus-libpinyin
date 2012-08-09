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

#include "PYStrokeEditor.h"
#include <string.h>
#include <string>
#include <vector>
#include <libintl.h>
#include <glib.h>
#include <sqlite3.h>
#include "PYString.h"
#include "PYConfig.h"

#define _(text) (gettext (text))

namespace PY {

class StrokeDatabase{
public:
    StrokeDatabase(){
        m_sqlite = NULL;
        m_sql = "";
    }

    ~StrokeDatabase(){
        if (m_sqlite){
            sqlite3_close (m_sqlite);
            m_sqlite = NULL;
        }
        m_sql = "";
    }

    gboolean isDatabaseExisted(const char *filename) {
        gboolean result = g_file_test(filename, G_FILE_TEST_IS_REGULAR);
        if (!result)
            return FALSE;

        sqlite3 *tmp_db = NULL;
        if (sqlite3_open_v2 (filename, &tmp_db,
                             SQLITE_OPEN_READONLY, NULL) != SQLITE_OK){
            return FALSE;
        }

        /* Check the desc table */
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;
        m_sql = "SELECT value FROM desc WHERE name = 'version';";
        result = sqlite3_prepare_v2 (tmp_db, m_sql.c_str(), -1, &stmt, &tail);
        if (result != SQLITE_OK)
            return FALSE;

        result = sqlite3_step (stmt);
        if (result != SQLITE_ROW)
            return FALSE;

        result = sqlite3_column_type (stmt, 0);
        if (result != SQLITE_TEXT)
            return FALSE;

        const char *version = (const char *) sqlite3_column_text (stmt, 0);
        if (strcmp("1.2.0", version) != 0)
            return FALSE;

        result = sqlite3_finalize (stmt);
        g_assert (result == SQLITE_OK);
        sqlite3_close (tmp_db);
        return TRUE;
    }

    /* No self-learning here, and no user database file. */
    gboolean openDatabase(const char *system_db) {
        if (!isDatabaseExisted (system_db))
            return FALSE;

        /* open system database. */
        if (sqlite3_open_v2 (system_db, &m_sqlite,
                             SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
            m_sqlite = NULL;
            return FALSE;
        }

        return TRUE;
    }

    /* List the characters in sequence order. */
    gboolean listCharacters(const char *prefix,
                            std::vector<std::string> & characters){
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;
        characters.clear ();

        /* list characters */
        const char *SQL_DB_LIST =
            "SELECT \"character\", \"token\" FROM \"strokes\""
            "WHERE \"strokes\" LIKE \"%s%\" ORDER BY \"sequence\" ASC;";
        m_sql.printf (SQL_DB_LIST, prefix);
        int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str(), -1, &stmt, &tail);
        if (result != SQLITE_OK)
            return FALSE;

        result = sqlite3_step (stmt);
        while (result == SQLITE_ROW){
            /* get the characters. */
            result = sqlite3_column_type (stmt, 0);
            if (result != SQLITE_TEXT)
                return FALSE;

            const char *character = (const char *)sqlite3_column_text (stmt, 0);
            characters.push_back (character);

            result = sqlite3_step (stmt);
        }

        sqlite3_finalize (stmt);
        if (result != SQLITE_DONE)
            return FALSE;
        return TRUE;
    }
private:
    sqlite3 *m_sqlite;
    String m_sql;
};

StrokeEditor::StrokeEditor (PinyinProperties &props, Config &config)
    : Editor (props, config)
{
    m_stroke_database = new StrokeDatabase;

    gboolean result = m_stroke_database->openDatabase
        (".." G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "strokes.db") ||
        m_stroke_database->openDatabase
        (PKGDATADIR G_DIR_SEPARATOR_S "db" G_DIR_SEPARATOR_S "strokes.db");

    if (!result)
        g_warning ("can't open strokes database.\n");
}

StrokeEditor::~StrokeEditor ()
{
    delete m_stroke_database;
    m_stroke_database = NULL;
}

gboolean
StrokeEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
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
        g_return_val_if_fail ('u' == keyval, FALSE);
        m_text = "u";
        m_cursor ++;
    } else {
        g_return_val_if_fail ('u' == m_text[0], FALSE);
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
StrokeEditor::processEditKey (guint keyval)
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
StrokeEditor::processPageKey (guint keyval)
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
StrokeEditor::processLabelKey (guint keyval)
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
StrokeEditor::processEnter (guint keyval)
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
StrokeEditor::processSpace (guint keyval)
{
    if (!(keyval == IBUS_space || keyval == IBUS_KP_Space))
        return FALSE;

    guint cursor_pos = m_lookup_table.cursorPos ();
    return selectCandidate (cursor_pos);
}

void
StrokeEditor::candidateClicked (guint index, guint button, guint state)
{
    selectCandidateInPage (index);
}

gboolean
StrokeEditor::selectCandidateInPage (guint index)
{
    guint page_size = m_lookup_table.pageSize ();
    guint cursor_pos = m_lookup_table.cursorPos ();

    if (G_UNLIKELY (index >= page_size))
        return FALSE;
    index += (cursor_pos / page_size) * page_size;

    return selectCandidate (index);
}

gboolean
StrokeEditor::selectCandidate (guint index)
{
    if (index >= m_lookup_table.size ())
        return FALSE;

    IBusText *candidate = m_lookup_table.getCandidate (index);
    Text text (candidate);
    commitText (text);
    reset ();
    return TRUE;
}

gboolean
StrokeEditor::updateStateFromInput (void)
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

    if ('u' != m_text[0]) {
        g_warning ("u is expected in m_text string.\n");
        m_auxiliary_text = "";
        clearLookupTable ();
        return FALSE;
    }

    m_auxiliary_text = "u";
    if (1 == m_text.length ()) {
        clearLookupTable ();

        const char * help_string =
            _("Please use \"hspnz\" to input.");
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
    std::vector<std::string> characters;
    gboolean retval = m_stroke_database->listCharacters
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
StrokeEditor::pageUp (void)
{
    if (G_LIKELY (m_lookup_table.pageUp ())) {
        update ();
    }
}

void
StrokeEditor::pageDown (void)
{
    if (G_LIKELY (m_lookup_table.pageDown ())) {
        update ();
    }
}

void
StrokeEditor::cursorUp (void)
{
    if (G_LIKELY (m_lookup_table.cursorUp ())) {
        update ();
    }
}

void
StrokeEditor::cursorDown (void)
{
    if (G_LIKELY (m_lookup_table.cursorDown ())) {
        update ();
    }
}

void
StrokeEditor::update (void)
{
    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}

void
StrokeEditor::reset (void)
{
    m_text = "";
    updateStateFromInput ();
    update ();
}

void
StrokeEditor::clearLookupTable (void)
{
    m_lookup_table.clear ();
    m_lookup_table.setPageSize (m_config.pageSize ());
    m_lookup_table.setOrientation (m_config.orientation ());
}

void
StrokeEditor::updateLookupTable (void)
{
    if (m_lookup_table.size ()){
        Editor::updateLookupTableFast (m_lookup_table, TRUE);
    } else {
        hideLookupTable ();
    }
}

void
StrokeEditor::updatePreeditText (void)
{
    if (G_UNLIKELY (m_preedit_text.empty ())) {
        hidePreeditText ();
        return;
    }

    StaticText preedit_text (m_preedit_text);
    Editor::updatePreeditText (preedit_text, m_cursor, TRUE);
}

void
StrokeEditor::updateAuxiliaryText (void)
{
    if (G_UNLIKELY (m_auxiliary_text.empty ())) {
        hideAuxiliaryText ();
        return;
    }

    StaticText aux_text (m_auxiliary_text);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}

gboolean
StrokeEditor::removeCharBefore (void)
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
StrokeEditor::removeCharAfter (void)
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

#if 0

/* using static initializor to test stroke database here. */
static class TestStrokeDatabase{
public:
    TestStrokeDatabase (){
        StrokeDatabase *db = new StrokeDatabase ();
        bool retval = db->isDatabaseExisted ("../data/strokes.db");
        g_assert (retval);
        retval = db->openDatabase ("../data/strokes.db");
        g_assert (retval);
        std::vector<std::string> chars;
        std::vector<std::string>::iterator iter;
        db->listCharacters("hshshhh", chars);
        printf ("characters:\t");
        for (iter = chars.begin(); iter != chars.end(); ++iter)
            printf ("%s ", iter->c_str());
        printf ("\n");
        printf ("stroke database test ok.\n");
    }
} test_stroke_database;

#endif
};
