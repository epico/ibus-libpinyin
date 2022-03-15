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

#ifndef __PY_TABLE_DATABASE_
#define __PY_TABLE_DATABASE_

#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include <vector>
#include "PYString.h"
#include "PYUtil.h"

namespace PY {

class TableDatabase{
public:
    static void init ();
    static TableDatabase & systemInstance (void) { return *m_system_instance; }
    static TableDatabase & userInstance (void)   { return *m_user_instance; }

public:
    TableDatabase();
    ~TableDatabase();

public:
    gboolean isDatabaseExisted(const char *filename);
    gboolean createDatabase(const char *filename);

    gboolean openDatabase(const char *filename, gboolean writable);
    gboolean listPhrases(const char *prefix,
                         std::vector<std::string> & phrases);

    gboolean getPhraseInfo(const char *phrase, int & freq);
    gboolean updatePhrase(const char *phrase, int freq);
    gboolean deletePhrase(const char *phrase, int freq);

    gboolean importTable (const char *filename);
    gboolean exportTable (const char *filename);
    gboolean clearTable ();

private:
    gboolean executeSQL(sqlite3 *sqlite);

private:
    sqlite3 *m_sqlite;
    String m_sql;

private:
    static std::unique_ptr<TableDatabase> m_system_instance;
    static std::unique_ptr<TableDatabase> m_user_instance;
};

};

#endif
