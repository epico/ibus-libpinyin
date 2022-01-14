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

#ifndef __PY_ENGLISH_DATABASE_
#define __PY_ENGLISH_DATABASE_

class EnglishDatabase{
public:
    EnglishDatabase();
    ~EnglishDatabase();

    gboolean isDatabaseExisted(const char *filename);
    gboolean createDatabase(const char *filename);

    gboolean openDatabase(const char *system_db, const char *user_db);
    gboolean listWords(const char *prefix, std::vector<std::string> & words);
    gboolean getWordInfo(const char *word, float & freq);
    gboolean insertWord(const char *word, float freq);

private:
    gboolean executeSQL(sqlite3 *sqlite);
    gboolean loadUserDB (void);
    gboolean saveUserDB (void);
    void modified (void);
    static gboolean timeoutCallback (gpointer data);
};

#endif
