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

#include "PYTableDatabase.h"

namespace PY {

std::unique_ptr<TableDatabase> TableDatabase::m_system_instance;
std::unique_ptr<TableDatabase> TableDatabase::m_user_instance;

void
TableDatabase::init ()
{
    /* system table database */
    if (m_system_instance.get () == NULL) {
        m_system_instance.reset (new TableDatabase ());
    }

    gboolean result = m_system_instance->openDatabase
        (".." G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "table.db", FALSE) ||
        m_system_instance->openDatabase
        (PKGDATADIR G_DIR_SEPARATOR_S "db" G_DIR_SEPARATOR_S "table.db", FALSE);

    if (!result)
        g_warning ("can't open system table database.\n");

    /* user table database */
    if (m_user_instance.get () == NULL) {
        m_user_instance.reset (new TableDatabase ());
    }

    gchar *path = g_build_filename (g_get_user_cache_dir (),
                                    "ibus", "libpinyin", "table-user.db", NULL);

    if (m_user_instance->isDatabaseExisted (path))
        result = m_user_instance->openDatabase (path, TRUE);
    else
        result = m_user_instance->createDatabase (path);

    if (!result)
        g_warning ("can't open user table database.\n");
}

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
        "freq INTEGER NOT NULL DEFAULT (10)"
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
    if (sqlite3_open_v2 (filename, &m_sqlite,
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
TableDatabase::importTable (const char *filename){
    /* Import the table into user database. */
    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;

    FILE *input = fopen (filename, "r");
    if (input == NULL)
        return FALSE;

    /* Get the next id, which is "MAX(id) + 1". */
    const char *SQL_DB_SELECT =
        "SELECT MAX(id) FROM phrases;";
    m_sql = SQL_DB_SELECT;
    int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str (), -1, &stmt, &tail);
    if (result != SQLITE_OK)
        return FALSE;

    result = sqlite3_step (stmt);
    if (result != SQLITE_ROW)
        return FALSE;

    result = sqlite3_column_type (stmt, 0);
    if (result != SQLITE_INTEGER)
        return FALSE;
    int id = sqlite3_column_int (stmt, 0);
    /* Open the table file with format:
       "tabkeys phrase freq". */

    while (!feof (input)) {
        ++id;
        char tabkeys[256], phrase[256];
        int freq = 10;
        fscanf (input, "%255s %255s %d\n", tabkeys, phrase, &freq);
        if (feof(input))
            break;

        const char *SQL_DB_REPLACE =
            "INSERT OR REPLACE INTO phrases (id, tabkeys, phrase, freq) "
            "VALUES (%d, \"%s\", \"%s\", %d);";

        m_sql.printf (SQL_DB_REPLACE, id, tabkeys, phrase, freq);
        gboolean retval = executeSQL (m_sqlite);
        if (!retval)
            break;
    }

    fclose (input);
    return TRUE;
}

gboolean
TableDatabase::exportTable (const char *filename){
    /* Export the content of user database. */
    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;

    /* Get the content of phrases table by "id" order. */
    const char *SQL_DB_SELECT =
        "SELECT tabkeys, phrase, freq FROM phrases "
        "ORDER BY id ASC;";
    m_sql = SQL_DB_SELECT;
    int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str (), -1, &stmt, &tail);
    if (result != SQLITE_OK)
        return FALSE;

    /* Write the table file with format:
       "tabkeys phrase freq". */
    FILE *output = fopen (filename, "w");
    if (output == NULL)
        return FALSE;

    result = sqlite3_step (stmt);
    while (result == SQLITE_ROW){
        /* write one line. */
        result = sqlite3_column_type (stmt, 0);
        if (result != SQLITE_TEXT)
            return FALSE;
        const char *tabkeys = (const char *)sqlite3_column_text (stmt, 0);

        result = sqlite3_column_type (stmt, 1);
        if (result != SQLITE_TEXT)
            return FALSE;
        const char *phrase = (const char *)sqlite3_column_text (stmt, 1);

        result = sqlite3_column_type (stmt, 2);
        if (result != SQLITE_INTEGER)
            return FALSE;
        const int freq = sqlite3_column_int (stmt, 2);

        fprintf (output, "%s\t%s\t%d\n", tabkeys, phrase, freq);

        result = sqlite3_step (stmt);
    }

    fclose (output);

    sqlite3_finalize (stmt);
    if (result != SQLITE_DONE)
        return FALSE;
    return TRUE;
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
