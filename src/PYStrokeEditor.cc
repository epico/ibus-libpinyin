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
#include <glib.h>
#include <sqlite3.h>
#include "PYString.h"

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

    String preedit = m_text.substr (1);
    Text text (preedit);
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
