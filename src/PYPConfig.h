/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
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
#ifndef __PY_LIBPINYIN_CONFIG_H_
#define __PY_LIBPINYIN_CONFIG_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <ibus.h>
#include "PYUtil.h"
#include "PYObject.h"
#include "PYConfig.h"
#include <pinyin.h>

namespace PY {

class LibPinyinConfig : public Config {
protected:
    LibPinyinConfig (const std::string & name);
    virtual ~LibPinyinConfig (void);

public:
    virtual gboolean luaConverter (std::string converter);
    virtual gboolean networkDictionaryStartTimestamp (gint64 timestamp);
    virtual gboolean networkDictionaryEndTimestamp (gint64 timestamp);

protected:
    void initDefaultValues (void);

    virtual void readDefaultValues (void);
    virtual gboolean valueChanged (const std::string &schema_id,
                                   const std::string &name,
                                   GVariant          *value);
private:
    static void valueChangedCallback (GSettings      *settings,
                                      const gchar    *name,
                                      LibPinyinConfig *self);

protected:
};

/* PinyinConfig */
class PinyinConfig : public LibPinyinConfig {
public:
    static void init ();
    static PinyinConfig & instance (void) { return *m_instance; }

protected:
    PinyinConfig ();
    virtual void readDefaultValues (void);

    virtual gboolean valueChanged (const std::string &schema_id,
                                   const std::string &name,
                                   GVariant          *value);

private:
    static std::unique_ptr<PinyinConfig> m_instance;
};

/* Bopomof Config */
class BopomofoConfig : public LibPinyinConfig {
public:
    static void init ();
    static BopomofoConfig & instance (void) { return *m_instance; }

protected:
    BopomofoConfig ();
    virtual void readDefaultValues (void);

    virtual gboolean valueChanged (const std::string &schema_id,
                                   const std::string &name,
                                   GVariant          *value);

private:
    static std::unique_ptr<BopomofoConfig> m_instance;
};

};
#endif
