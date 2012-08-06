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

const gchar * const CONFIG_CORRECT_PINYIN            = "CorrectPinyin";
const gchar * const CONFIG_FUZZY_PINYIN              = "FuzzyPinyin";
const gchar * const CONFIG_ORIENTATION               = "LookupTableOrientation";
const gchar * const CONFIG_PAGE_SIZE                 = "LookupTablePageSize";
const gchar * const CONFIG_SHIFT_SELECT_CANDIDATE    = "ShiftSelectCandidate";
const gchar * const CONFIG_MINUS_EQUAL_PAGE          = "MinusEqualPage";
const gchar * const CONFIG_COMMA_PERIOD_PAGE         = "CommaPeriodPage";
const gchar * const CONFIG_AUTO_COMMIT               = "AutoCommit";
const gchar * const CONFIG_DOUBLE_PINYIN             = "DoublePinyin";
const gchar * const CONFIG_DOUBLE_PINYIN_SCHEMA      = "DoublePinyinSchema";
const gchar * const CONFIG_DOUBLE_PINYIN_SHOW_RAW    = "DoublePinyinShowRaw";
const gchar * const CONFIG_INIT_CHINESE              = "InitChinese";
const gchar * const CONFIG_INIT_FULL                 = "InitFull";
const gchar * const CONFIG_INIT_FULL_PUNCT           = "InitFullPunct";
const gchar * const CONFIG_INIT_SIMP_CHINESE         = "InitSimplifiedChinese";
const gchar * const CONFIG_SPECIAL_PHRASES           = "SpecialPhrases";
const gchar * const CONFIG_BOPOMOFO_KEYBOARD_MAPPING = "BopomofoKeyboardMapping";
const gchar * const CONFIG_SELECT_KEYS               = "SelectKeys";
const gchar * const CONFIG_GUIDE_KEY                 = "GuideKey";
const gchar * const CONFIG_AUXILIARY_SELECT_KEY_F    = "AuxiliarySelectKey_F";
const gchar * const CONFIG_AUXILIARY_SELECT_KEY_KP   = "AuxiliarySelectKey_KP";
const gchar * const CONFIG_ENTER_KEY                 = "EnterKey";



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
    m_shift_select_candidate = FALSE;
    m_minus_equal_page = TRUE;
    m_comma_period_page = TRUE;
    m_auto_commit = FALSE;

    m_double_pinyin = FALSE;
    m_double_pinyin_schema = 0;
    m_double_pinyin_show_raw = FALSE;

    m_init_chinese = TRUE;
    m_init_full = FALSE;
    m_init_full_punct = TRUE;
    m_init_simp_chinese = TRUE;
    m_special_phrases = TRUE;
}


void
Config::readDefaultValues (void)
{
#if defined(HAVE_IBUS_CONFIG_GET_VALUES)
    /* read all values together */
    initDefaultValues ();
    GVariant *values =
            ibus_config_get_values (get<IBusConfig> (), m_section.c_str ());
    g_return_if_fail (values != NULL);

    GVariantIter iter;
    gchar *name;
    GVariant *value;
    g_variant_iter_init (&iter, values);
    while (g_variant_iter_next (&iter, "{sv}", &name, &value)) {
        valueChanged (m_section, name, value);
        g_free (name);
        g_variant_unref (value);
    }
    g_variant_unref (values);
#else
    /* others */
    m_orientation = read (CONFIG_ORIENTATION, 0);
    if (m_orientation != IBUS_ORIENTATION_VERTICAL &&
        m_orientation != IBUS_ORIENTATION_HORIZONTAL) {
        m_orientation = IBUS_ORIENTATION_HORIZONTAL;
        g_warn_if_reached ();
    }
    m_page_size = read (CONFIG_PAGE_SIZE, 5);
    if (m_page_size > 10) {
        m_page_size = 5;
        g_warn_if_reached ();
    }

#endif
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

static inline bool
normalizeGVariant (GVariant *value, bool defval)
{
    if (value == NULL || g_variant_classify (value) != G_VARIANT_CLASS_BOOLEAN)
        return defval;
    return g_variant_get_boolean (value);
}

static inline gint
normalizeGVariant (GVariant *value, gint defval)
{
    if (value == NULL || g_variant_classify (value) != G_VARIANT_CLASS_INT32)
        return defval;
    return g_variant_get_int32 (value);
}

static inline std::string
normalizeGVariant (GVariant *value, const std::string &defval)
{
    if (value == NULL || g_variant_classify (value) != G_VARIANT_CLASS_STRING)
        return defval;
    return g_variant_get_string (value, NULL);
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
