/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

class Bus;

class LibPinyinConfig : public Config {
protected:
    LibPinyinConfig (Bus & bus, const std::string & name);
    virtual ~LibPinyinConfig (void);

public:

protected:
    void initDefaultValues (void);

    virtual void readDefaultValues (void);
    virtual gboolean valueChanged (const std::string &section,
                                   const std::string &name,
                                   GVariant          *value);
private:
    static void valueChangedCallback (IBusConfig     *config,
                                      const gchar    *section,
                                      const gchar    *name,
                                      GVariant       *value,
                                      LibPinyinConfig *self);

protected:
};

/* PinyinConfig */
class LibPinyinPinyinConfig : public LibPinyinConfig {
public:
    static void init (Bus & bus);
    static LibPinyinPinyinConfig & instance (void) { return *m_instance; }

protected:
    LibPinyinPinyinConfig (Bus & bus);
    virtual void readDefaultValues (void);

    virtual gboolean valueChanged (const std::string &section,
                                   const std::string &name,
                                   GVariant          *value);

private:
    static std::unique_ptr<LibPinyinPinyinConfig> m_instance;
};

/* Bopomof Config */
class LibPinyinBopomofoConfig : public LibPinyinConfig {
public:
    static void init (Bus & bus);
    static LibPinyinBopomofoConfig & instance (void) { return *m_instance; }

protected:
    LibPinyinBopomofoConfig (Bus & bus);
    virtual void readDefaultValues (void);

    virtual gboolean valueChanged (const std::string &section,
                                   const std::string &name,
                                   GVariant          *value);

private:
    static std::unique_ptr<LibPinyinBopomofoConfig> m_instance;
};

};
#endif
