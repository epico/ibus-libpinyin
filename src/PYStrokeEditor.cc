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
