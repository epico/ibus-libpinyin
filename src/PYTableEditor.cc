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
#include "PYString.h"
#include "PYConfig.h"

#define _(text) (gettext (text))

namespace PY {

TableDatabase::TableDatabase(){
    m_sqlite = NULL;
    m_sql = "";
}

TableDatabase::~TableDatabase(){
    if (m_sqlite){
        sqlite3_close (m_sqlite);
        m_sqlite = NULL;
    }
    m_sql = "";
}

gboolean
TableDatabase::isDatabaseExisted(const char *filename) {
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
    if (strcmp("1.12.0", version) != 0)
        return FALSE;

    result = sqlite3_finalize (stmt);
    g_assert (result == SQLITE_OK);
    sqlite3_close (tmp_db);
    return TRUE;
}

gboolean
TableDatabase::createDatabase(const char *filename) {
    /* unlink the old database. */
    gboolean retval = g_file_test (filename, G_FILE_TEST_IS_REGULAR);
    if (retval) {
        int result = g_unlink (filename);
        if (result == -1)
            return FALSE;
    }

    char *dirname = g_path_get_dirname (filename);
    g_mkdir_with_parents (dirname, 0700);
    g_free (dirname);

    sqlite3 *tmp_db = NULL;
    if (sqlite3_open_v2 (filename, &tmp_db,
                         SQLITE_OPEN_READWRITE |
                         SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
        return FALSE;
    }

    /* Create DESCription table */
    m_sql = "BEGIN TRANSACTION;\n";
    m_sql << "CREATE TABLE IF NOT EXISTS desc (name TEXT PRIMARY KEY, value TEXT);\n";
    m_sql << "INSERT OR IGNORE INTO desc VALUES ('version', '1.12.0');";
    m_sql << "COMMIT;\n";

    if (!executeSQL (tmp_db)) {
        sqlite3_close (tmp_db);
        return FALSE;
    }

    /* Create Schema */
    m_sql = "CREATE TABLE IF NOT EXISTS phrases ( "
        "id INTEGER PRIMARY KEY NOT NULL,"
        "tabkeys TEXT NOT NULL,"
        "phrase TEXT NOT NULL UNIQUE,"
        "freq INTEGER NOT NULL DEFAULT (0)"
        ");";
    if (!executeSQL (tmp_db)) {
        sqlite3_close (tmp_db);
        return FALSE;
    }
    return TRUE;
}

/* No self-learning here, and no user database file. */
gboolean
TableDatabase::openDatabase(const char *filename, gboolean writable) {
    int flags = SQLITE_OPEN_READONLY;

    if (writable)
        flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    /* open system database. */
    if (sqlite3_open_v2 (system_db, &m_sqlite,
                         flags, NULL) != SQLITE_OK) {
        m_sqlite = NULL;
        return FALSE;
    }

    return TRUE;
}

/* List the phrases in frequency and id order. */
gboolean
TableDatabase::listPhrases(const char *prefix,
                           std::vector<std::string> & phrases){
    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;
    phrases.clear ();

    /* list phrases */
    const char *SQL_DB_LIST =
        "SELECT phrase FROM phrases "
        "WHERE tabkeys LIKE \"%s%\" ORDER BY freq DESC, id ASC;";
    m_sql.printf (SQL_DB_LIST, prefix);
    int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str(), -1, &stmt, &tail);
    if (result != SQLITE_OK)
        return FALSE;

    result = sqlite3_step (stmt);
    while (result == SQLITE_ROW){
        /* get the phrases. */
        result = sqlite3_column_type (stmt, 0);
        if (result != SQLITE_TEXT)
            return FALSE;

        const char *phrase = (const char *)sqlite3_column_text (stmt, 0);
        phrases.push_back (phrase);

        result = sqlite3_step (stmt);
    }

    sqlite3_finalize (stmt);
    if (result != SQLITE_DONE)
        return FALSE;
    return TRUE;
}

gboolean
TableDatabase::getPhraseInfo(const char *phrase, int & freq){
    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;

    /* get phrase info */
    const char *SQL_DB_SELECT =
        "SELECT freq FROM phrases WHERE phrase = \"%s\";";
    m_sql.printf (SQL_DB_SELECT, phrase);
    int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str (), -1, &stmt, &tail);
    g_assert (result == SQLITE_OK);
    result = sqlite3_step (stmt);
    if (result != SQLITE_ROW)
        return FALSE;
    result = sqlite3_column_type (stmt, 0);
    if (result != SQLITE_INTEGER)
        return FALSE;
    freq = sqlite3_column_int (stmt, 0);
    result = sqlite3_finalize (stmt);
    g_assert (result == SQLITE_OK);
    return TRUE;
}

gboolean
TableDatabase::updatePhrase(const char *phrase, int freq){
    const char *SQL_DB_UPDATE =
        "UPDATE phrases SET freq = \"%d\" WHERE phrase = \"%s\";";
    m_sql.printf (SQL_DB_UPDATE, freq, phrase);
    gboolean retval = executeSQL (m_sqlite);
    return retval;
}

gboolean
TableDatabase::deletePhrase(const char *phrase, int freq){
    const char *SQL_DB_DELETE =
        "DELETE FROM phrases WHERE phrase = \"%s\";";
    m_sql.printf (SQL_DB_DELETE, phrase);
    gboolean retval = executeSQL (m_sqlite);
    return retval;
}

gboolean
TableDatabase::clearTable (){
    const char *SQL_DB_DELETE =
        "DELETE FROM phrases;";
    m_sql = SQL_DB_DELETE;
    gboolean retval = executeSQL (m_sqlite);
    return retval;
}

gboolean
TableDatabase::executeSQL(sqlite3 *sqlite){
    gchar *errmsg = NULL;
    if (sqlite3_exec (sqlite, m_sql.c_str (), NULL, NULL, &errmsg)
        != SQLITE_OK) {
        g_warning ("%s: %s", errmsg, m_sql.c_str());
        sqlite3_free (errmsg);
        return FALSE;
    }
    m_sql.clear ();
    return TRUE;
}

TableEditor::TableEditor (PinyinProperties &props, Config &config)
    : Editor (props, config)
{
    m_table_database = new TableDatabase;

    gboolean result = m_table_database->openDatabase
        (".." G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "table.db") ||
        m_table_database->openDatabase
        (PKGDATADIR G_DIR_SEPARATOR_S "db" G_DIR_SEPARATOR_S "table.db");

    if (!result)
        g_warning ("can't open table database.\n");
}

TableEditor::~TableEditor ()
{
    delete m_table_database;
    m_table_database = NULL;
}

gboolean
TableEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
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
    commitText (text);
    reset ();
    return TRUE;
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
    gboolean retval = m_table_database->listPhrases
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

#if 0

/* using static initializor to test table database here. */
static class TestTableDatabase{
public:
    TestTableDatabase (){
        TableDatabase *db = new TableDatabase ();
        bool retval = db->isDatabaseExisted ("../data/table.db");
        g_assert (retval);
        retval = db->openDatabase ("../data/table.db");
        g_assert (retval);
        std::vector<std::string> chars;
        std::vector<std::string>::iterator iter;
        db->listCharacters("hshshhh", chars);
        printf ("characters:\t");
        for (iter = chars.begin(); iter != chars.end(); ++iter)
            printf ("%s ", iter->c_str());
        printf ("\n");
        printf ("table database test ok.\n");
    }
} test_table_database;

#endif
};
