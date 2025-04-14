/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2021 Peng Wu <alexepico@gmail.com>
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

#include "PYEnglishDatabase.h"
#include <glib.h>
#include <glib/gstdio.h>

namespace PY{

#define DB_BACKUP_TIMEOUT   (60)

std::unique_ptr<EnglishDatabase> EnglishDatabase::m_instance;

void
EnglishDatabase::init ()
{
    if (m_instance.get () == NULL) {
        m_instance.reset (new EnglishDatabase ());
    }

    gchar *path = g_build_filename (g_get_user_cache_dir (),
                                     "ibus", "libpinyin", "english-user.db", NULL);

    gboolean result = m_instance->openDatabase
        (".." G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "english.db",
         "english-user.db") ||
        m_instance->openDatabase
        (PKGDATADIR G_DIR_SEPARATOR_S "db" G_DIR_SEPARATOR_S "english.db", path);
    if (!result)
        g_warning ("can't open English word list database.\n");

    g_free (path);
}



EnglishDatabase::EnglishDatabase(){
    m_sqlite = NULL;
    m_sql = "";
    m_user_db = NULL;
    m_timeout_id = 0;
    m_timer = g_timer_new ();
}

EnglishDatabase::~EnglishDatabase(){
    g_timer_destroy (m_timer);
    if (m_timeout_id != 0) {
        saveUserDB ();
        g_source_remove (m_timeout_id);
    }

    if (m_sqlite){
        sqlite3_close (m_sqlite);
        m_sqlite = NULL;
    }
    m_sql = "";
    g_free (m_user_db);
    m_user_db = NULL;
}

gboolean
EnglishDatabase::isDatabaseExisted(const char *filename) {
    gboolean result = g_file_test (filename, G_FILE_TEST_IS_REGULAR);
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
    if (strcmp("1.2.0", version ) != 0)
        return FALSE;

    result = sqlite3_finalize (stmt);
    g_assert (result == SQLITE_OK);
    sqlite3_close (tmp_db);
    return TRUE;
}

gboolean
EnglishDatabase::createDatabase(const char *filename) {
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
    m_sql << "INSERT OR IGNORE INTO desc VALUES ('version', '1.2.0');";
    m_sql << "COMMIT;\n";

    if (!executeSQL (tmp_db)) {
        sqlite3_close (tmp_db);
        return FALSE;
    }

    /* Create Schema */
    m_sql = "CREATE TABLE IF NOT EXISTS english ("
        "word TEXT NOT NULL PRIMARY KEY,"
        "freq FLOAT NOT NULL DEFAULT(0)"
        ");";
    if (!executeSQL (tmp_db)) {
        sqlite3_close (tmp_db);
        return FALSE;
    }
    return TRUE;
}

gboolean
EnglishDatabase::openDatabase(const char *system_db, const char *user_db){
    if (!isDatabaseExisted (system_db))
        return FALSE;
    if (!isDatabaseExisted (user_db)) {
        gboolean result = createDatabase (user_db);
        if (!result)
            return FALSE;
    }
    /* cache the user db name. */
    m_user_db = g_strdup (user_db);

    /* do database attach here. :) */
    if (sqlite3_open_v2 (system_db, &m_sqlite,
                         SQLITE_OPEN_READWRITE |
                         SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
        m_sqlite = NULL;
        return FALSE;
    }

#if 0
    m_sql.printf (SQL_ATTACH_DB, user_db);
    if (!executeSQL (m_sqlite)) {
        sqlite3_close (m_sqlite);
        m_sqlite = NULL;
        return FALSE;
    }
    return TRUE;
#endif
    return loadUserDB();
}

gboolean
EnglishDatabase::hasWord(const char *word){
    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;

    /* match word */
    const char *SQL_DB_MATCH =
        "SELECT word FROM english WHERE word = \"%s\" UNION ALL "
        "SELECT word FROM userdb.english WHERE word = \"%s\";";
    m_sql.printf (SQL_DB_MATCH, word, word);
    int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str(), -1, &stmt, &tail);
    if (result != SQLITE_OK)
        return FALSE;

    int count = 0;
    result = sqlite3_step (stmt);
    while (result == SQLITE_ROW) {
        /* count the match */
        result = sqlite3_column_type (stmt, 0);
        if (result != SQLITE_TEXT)
            return FALSE;

        const char *text = (const char *)sqlite3_column_text (stmt, 0);
        g_assert (0 == strcmp (word, text));
        ++count;
        result = sqlite3_step (stmt);
    }

    sqlite3_finalize (stmt);
    if (result != SQLITE_DONE)
        return FALSE;

    return count > 0;
}

/* List the words in freq order. */
gboolean
EnglishDatabase::listWords(const char *prefix, std::vector<std::string> & words){
    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;
    words.clear ();

    /* list words */
    const char *SQL_DB_LIST =
        "SELECT word FROM ( "
        "SELECT * FROM english UNION ALL SELECT * FROM userdb.english) "
        " WHERE word GLOB \"%s*\" GROUP BY word ORDER BY SUM(freq) DESC;";
    m_sql.printf (SQL_DB_LIST, prefix);
    int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str(), -1, &stmt, &tail);
    if (result != SQLITE_OK)
        return FALSE;

    result = sqlite3_step (stmt);
    while (result == SQLITE_ROW){
        /* get the words. */
        result = sqlite3_column_type (stmt, 0);
        if (result != SQLITE_TEXT)
            return FALSE;

        const char *word = (const char *)sqlite3_column_text (stmt, 0);
        words.push_back (word);
        result = sqlite3_step (stmt);
    }

    sqlite3_finalize (stmt);
    if (result != SQLITE_DONE)
        return FALSE;
    return TRUE;
}

/* Get the freq of user sqlite db. */
gboolean
EnglishDatabase::getUserWordInfo(const char *word, float & freq){
    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;
    /* get word info. */
    const char *SQL_DB_SELECT =
        "SELECT freq FROM userdb.english WHERE word = \"%s\";";
    m_sql.printf (SQL_DB_SELECT, word);
    int result = sqlite3_prepare_v2 (m_sqlite, m_sql.c_str(), -1, &stmt, &tail);
    g_assert (result == SQLITE_OK);
    result = sqlite3_step (stmt);
    if (result != SQLITE_ROW)
        return FALSE;
    result = sqlite3_column_type (stmt, 0);
    if (result != SQLITE_FLOAT)
        return FALSE;
    freq = sqlite3_column_double (stmt, 0);
    result = sqlite3_finalize (stmt);
    g_assert (result == SQLITE_OK);
    return TRUE;
}

/* Update the freq with delta value. */
gboolean
EnglishDatabase::updateUserWord(const char *word, float freq){
    const char *SQL_DB_UPDATE =
        "UPDATE userdb.english SET freq = \"%f\" WHERE word = \"%s\";";
    m_sql.printf (SQL_DB_UPDATE, freq, word);
    gboolean retval =  executeSQL (m_sqlite);
    modified ();
    return retval;
}

/* Insert the word into user db with the initial freq. */
gboolean
EnglishDatabase::insertUserWord(const char *word, float freq){
    const char *SQL_DB_INSERT =
        "INSERT INTO userdb.english (word, freq) VALUES (\"%s\", \"%f\");";
    m_sql.printf (SQL_DB_INSERT, word, freq);
    gboolean retval = executeSQL (m_sqlite);
    modified ();
    return retval;
}

gboolean
EnglishDatabase::deleteUserWord(const char *word){
    const char *SQL_DB_DELETE =
        "DELETE FROM userdb.english WHERE word = \"%s\";";
    m_sql.printf (SQL_DB_DELETE, word);
    gboolean retval = executeSQL (m_sqlite);
    modified ();
    return retval;
}

gboolean
EnglishDatabase::executeSQL(sqlite3 *sqlite){
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

gboolean
EnglishDatabase::loadUserDB (void){
    sqlite3 *userdb =  NULL;
    /* Attach user database */
    do {
        const char *SQL_ATTACH_DB =
            "ATTACH DATABASE ':memory:' AS userdb;";
        m_sql.printf (SQL_ATTACH_DB);
        if (!executeSQL (m_sqlite))
            break;

        /* Note: user db is always created by openDatabase. */
        if (sqlite3_open_v2 ( m_user_db, &userdb,
                              SQLITE_OPEN_READWRITE |
                              SQLITE_OPEN_CREATE, NULL) != SQLITE_OK)
            break;

        sqlite3_backup *backup = sqlite3_backup_init (m_sqlite, "userdb", userdb, "main");

        if (backup) {
            sqlite3_backup_step (backup, -1);
            sqlite3_backup_finish (backup);
        }

        sqlite3_close (userdb);
        return TRUE;
    } while (0);

    if (userdb)
        sqlite3_close (userdb);
    return FALSE;
}

gboolean
EnglishDatabase::saveUserDB (void){
    sqlite3 *userdb = NULL;
    String tmpfile = String(m_user_db) + "-tmp";
    do {
        /* remove tmpfile if it exist */
        g_unlink(tmpfile);

        if (sqlite3_open_v2 (tmpfile, &userdb,
                             SQLITE_OPEN_READWRITE |
                             SQLITE_OPEN_CREATE, NULL) != SQLITE_OK)
            break;

        sqlite3_backup *backup = sqlite3_backup_init (userdb, "main", m_sqlite, "userdb");

        if (backup == NULL)
            break;

        sqlite3_backup_step (backup, -1);
        sqlite3_backup_finish (backup);
        sqlite3_close (userdb);

        g_rename(tmpfile, m_user_db);
        return TRUE;
    } while (0);

    if (userdb)
        sqlite3_close (userdb);
    g_unlink (tmpfile);
    return FALSE;
}

void
EnglishDatabase::modified (void){
    /* Restart the timer */
    g_timer_start (m_timer);

    if (m_timeout_id != 0)
        return;

    m_timeout_id = g_timeout_add_seconds (DB_BACKUP_TIMEOUT,
                                          EnglishDatabase::timeoutCallback,
                                          static_cast<gpointer> (this));
}

gboolean
EnglishDatabase::timeoutCallback (gpointer data){
    EnglishDatabase *self = static_cast<EnglishDatabase *> (data);

    /* Get elapsed time since last modification of database. */
    guint elapsed = (guint) g_timer_elapsed (self->m_timer, NULL);

    if (elapsed >= DB_BACKUP_TIMEOUT &&
        self->saveUserDB ()) {
        self->m_timeout_id = 0;
        return FALSE;
    }

    return TRUE;
}

gboolean
EnglishDatabase::train (const char *word, float delta)
{
    float freq = 0;
    gboolean retval = getUserWordInfo (word, freq);
    if (retval) {
        freq += delta;
        updateUserWord (word, freq);
    } else {
        insertUserWord (word, delta);
    }
    return TRUE;
}

#if 0

/* using static initializor to test english database here. */
static class TestEnglishDatabase{
public:
    TestEnglishDatabase (){
        EnglishDatabase *db = new EnglishDatabase ();
        bool retval = db->isDatabaseExisted ("/tmp/english-user.db");
        g_assert (!retval);
        retval = db->createDatabase ("english-user.db");
        g_assert (retval);
        retval = db->openDatabase ("english.db", "english-user.db");
        g_assert (retval);
        retval = db->hasWord ("hello");
        printf ("has word hello:%d\n", retval);
        float freq = 0;
        retval = db->getUserWordInfo ("hello", freq);
        printf ("word hello:%d, %f.\n", retval, freq);
        if (retval) {
            db->updateUserWord ("hello", 0.1);
        } else {
            db->insertUserWord ("hello", 0.1);
        }
        retval = db->hasWord ("hello");
        printf ("has word hello:%d\n", retval);
        retval = db->getUserWordInfo ("hello", freq);
        printf ("word hello:%d, %f.\n", retval, freq);
        db->deleteUserWord ("hello");
        freq = 0;
        retval = db->getUserWordInfo ("hello", freq);
        printf ("word hello:%d, %f.\n", retval, freq);
        retval = db->hasWord ("hello");
        printf ("has word hello:%d\n", retval);
        printf ("english database test ok.\n");
    }
} test_english_database;

#endif

};
