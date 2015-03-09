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
#include "PYConfig.h"

#include "PYTypes.h"
#include "PYBus.h"

namespace PY {


Config::Config (Bus & bus, const std::string & name)
    : Object (ibus_bus_get_config (bus)),
      m_section ("engine/" + name)
{
    initDefaultValues ();
    g_signal_connect (get<IBusConfig> (),
                      "value-changed",
                      G_CALLBACK (valueChangedCallback),
                      this);
}

Config::~Config (void)
{
}

void
Config::initDefaultValues (void)
{
    m_orientation = IBUS_ORIENTATION_HORIZONTAL;
    m_page_size = 5;
    m_remember_every_input = FALSE;

    m_shift_select_candidate = FALSE;
    m_minus_equal_page = TRUE;
    m_comma_period_page = TRUE;
    m_auto_commit = FALSE;

    m_double_pinyin = FALSE;
    m_double_pinyin_schema = DOUBLE_PINYIN_DEFAULT;
    m_double_pinyin_show_raw = FALSE;

    m_init_chinese = TRUE;
    m_init_full = FALSE;
    m_init_full_punct = TRUE;
    m_init_simp_chinese = TRUE;
    m_special_phrases = TRUE;

    m_dictionaries = "2";

    m_main_switch = "<Shift>";
    m_letter_switch = "";
    m_punct_switch = "<Control>period";
    m_trad_switch = "<Control><Shift>f";
}


void
Config::readDefaultValues (void)
{
}

inline bool
Config::read (const gchar * name,
              bool          defval)
{
    GVariant *value = NULL;
    if ((value = ibus_config_get_value (get<IBusConfig> (), m_section.c_str (), name)) != NULL) {
        if (g_variant_classify (value) == G_VARIANT_CLASS_BOOLEAN)
            return g_variant_get_boolean (value);
    }

    // write default value to config
    value = g_variant_new ("b", defval);
    ibus_config_set_value (get<IBusConfig> (), m_section.c_str (), name, value);

    return defval;
}

inline gint
Config::read (const gchar * name,
              gint          defval)
{
    GVariant *value = NULL;
    if ((value = ibus_config_get_value (get<IBusConfig> (), m_section.c_str (), name)) != NULL) {
        if (g_variant_classify (value) == G_VARIANT_CLASS_INT32)
            return g_variant_get_int32 (value);
    }

    // write default value to config
    value = g_variant_new ("i", defval);
    ibus_config_set_value (get<IBusConfig> (), m_section.c_str (), name, value);

    return defval;
}

inline std::string
Config::read (const gchar * name,
              const gchar * defval)
{
    GVariant *value = NULL;
    if ((value = ibus_config_get_value (get<IBusConfig> (), m_section.c_str (), name)) != NULL) {
        if (g_variant_classify (value) == G_VARIANT_CLASS_STRING)
            return g_variant_get_string (value, NULL);
    }

    // write default value to config
    value = g_variant_new ("s", defval);
    ibus_config_set_value (get<IBusConfig> (), m_section.c_str (), name, value);

    return defval;
}

gboolean
Config::valueChanged (const std::string &section,
                      const std::string &name,
                      GVariant          *value)
{
    return FALSE;
}

void
Config::valueChangedCallback (IBusConfig  *config,
                              const gchar *section,
                              const gchar *name,
                              GVariant    *value,
                              Config      *self)
{
    self->valueChanged (section, name, value);
}

};
