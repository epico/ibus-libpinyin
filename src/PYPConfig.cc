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
#include "PYPConfig.h"

#include <string.h>
#include <pinyin.h>
#include "PYBus.h"
#include "PYLibPinyin.h"


namespace PY {

const gchar * const CONFIG_CORRECT_PINYIN            = "correct-pinyin";
const gchar * const CONFIG_FUZZY_PINYIN              = "fuzzy-pinyin";
const gchar * const CONFIG_ORIENTATION               = "lookup-table-orientation";
const gchar * const CONFIG_PAGE_SIZE                 = "lookup-table-page-size";
const gchar * const CONFIG_REMEMBER_EVERY_INPUT      = "remember-every-input";
const gchar * const CONFIG_SHIFT_SELECT_CANDIDATE    = "shift-select-candidate";
const gchar * const CONFIG_MINUS_EQUAL_PAGE          = "minus-equal-page";
const gchar * const CONFIG_COMMA_PERIOD_PAGE         = "comma-period-page";
const gchar * const CONFIG_AUTO_COMMIT               = "auto-commit";
const gchar * const CONFIG_DOUBLE_PINYIN             = "double-pinyin";
const gchar * const CONFIG_DOUBLE_PINYIN_SCHEMA      = "double-pinyin-schema";
const gchar * const CONFIG_DOUBLE_PINYIN_SHOW_RAW    = "double-pinyin-show-raw";
const gchar * const CONFIG_INIT_CHINESE              = "init-chinese";
const gchar * const CONFIG_INIT_FULL                 = "init-full";
const gchar * const CONFIG_INIT_FULL_PUNCT           = "init-full-punct";
const gchar * const CONFIG_INIT_SIMP_CHINESE         = "init-simplified-chinese";
const gchar * const CONFIG_SPECIAL_PHRASES           = "special-phrases";
const gchar * const CONFIG_DICTIONARIES              = "dictionaries";
const gchar * const CONFIG_BOPOMOFO_KEYBOARD_MAPPING = "bopomofo-keyboard-mapping";
const gchar * const CONFIG_SELECT_KEYS               = "select-keys";
const gchar * const CONFIG_GUIDE_KEY                 = "guide-key";
const gchar * const CONFIG_AUXILIARY_SELECT_KEY_F    = "auxiliary-select-key-f";
const gchar * const CONFIG_AUXILIARY_SELECT_KEY_KP   = "auxiliary-select-key-kp";
const gchar * const CONFIG_ENTER_KEY                 = "enter-key";
const gchar * const CONFIG_IMPORT_DICTIONARY         = "import-dictionary";
const gchar * const CONFIG_EXPORT_DICTIONARY         = "export-dictionary";
const gchar * const CONFIG_CLEAR_USER_DATA           = "clear-user-data";
/* const gchar * const CONFIG_CTRL_SWITCH               = "ctrl-switch"; */
const gchar * const CONFIG_MAIN_SWITCH               = "main-switch";
const gchar * const CONFIG_LETTER_SWITCH             = "letter-switch";
const gchar * const CONFIG_PUNCT_SWITCH              = "punct-switch";
const gchar * const CONFIG_TRAD_SWITCH               = "trad-switch";

const pinyin_option_t PINYIN_DEFAULT_OPTION =
        PINYIN_INCOMPLETE |
        ZHUYIN_INCOMPLETE|
        PINYIN_CORRECT_ALL|
        0;

std::unique_ptr<PinyinConfig> PinyinConfig::m_instance;
std::unique_ptr<BopomofoConfig> BopomofoConfig::m_instance;

LibPinyinConfig::LibPinyinConfig (Bus & bus, const std::string & name)
    : Config (bus, name)
{
    initDefaultValues ();
    g_signal_connect (get<IBusConfig> (),
                      "value-changed",
                      G_CALLBACK (valueChangedCallback),
                      this);
}

LibPinyinConfig::~LibPinyinConfig (void)
{
}

void
LibPinyinConfig::initDefaultValues (void)
{
    m_option = PINYIN_DEFAULT_OPTION;
    m_option_mask = PINYIN_INCOMPLETE | ZHUYIN_INCOMPLETE | PINYIN_CORRECT_ALL;

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

    m_bopomofo_keyboard_mapping = ZHUYIN_DEFAULT;

    m_dictionaries = "";

    m_main_switch = "<Shift>";
    m_letter_switch = "";
    m_punct_switch = "<Control>period";
    m_trad_switch = "<Control><Shift>f";
}

static const struct {
    const gchar * const name;
    guint option;
} options [] = {
    { "incomplete-pinyin",       PINYIN_INCOMPLETE|ZHUYIN_INCOMPLETE},
    /* fuzzy pinyin */
    { "fuzzy-pinyin-c-ch",       PINYIN_AMB_C_CH      },
    { "fuzzy-pinyin-z-zh",       PINYIN_AMB_Z_ZH      },
    { "fuzzy-pinyin-s-sh",       PINYIN_AMB_S_SH      },
    { "fuzzy-pinyin-l-n",        PINYIN_AMB_L_N       },
    { "fuzzy-pinyin-f-h",        PINYIN_AMB_F_H       },
    { "fuzzy-pinyin-l-r",        PINYIN_AMB_L_R       },
    { "fuzzy-pinyin-g-k",        PINYIN_AMB_G_K       },
    { "fuzzy-pinyin-an-ang",     PINYIN_AMB_AN_ANG    },
    { "fuzzy-pinyin-en-eng",     PINYIN_AMB_EN_ENG    },
    { "fuzzy-pinyin-in-ing",     PINYIN_AMB_IN_ING    },
    /* dynamic adjust */
    { "dynamic-adjust",          DYNAMIC_ADJUST       },
};

void
LibPinyinConfig::readDefaultValues (void)
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
        /* skip signals here. */
        if (0 == strcmp(CONFIG_IMPORT_DICTIONARY, name))
            continue;

        if (0 == strcmp(CONFIG_EXPORT_DICTIONARY, name))
            continue;

        if (0 == strcmp(CONFIG_CLEAR_USER_DATA, name))
            continue;

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
    m_remember_every_input = read (CONFIG_REMEMBER_EVERY_INPUT, false);

    m_dictionaries = read (CONFIG_DICTIONARIES, std::string (""));

    m_main_switch = read (CONFIG_MAIN_SWITCH, std::string ("<Shift>"));
    m_letter_switch = read (CONFIG_LETTER_SWITCH, std::string (""));
    m_punct_switch = read (CONFIG_PUNCT_SWITCH, std::string ("<Control>period"));
    m_trad_switch = read (CONFIG_TRAD_SWITCH, std::string ("<Control><Shift>f"));

    /* fuzzy pinyin */
    if (read (CONFIG_FUZZY_PINYIN, false))
        m_option_mask |= PINYIN_AMB_ALL;
    else
        m_option_mask &= ~PINYIN_AMB_ALL;

    /* read values */
    for (guint i = 0; i < G_N_ELEMENTS (options); i++) {
        if (read (options[i].name,
                  (options[i].option & PINYIN_DEFAULT_OPTION) != 0)) {
            m_option |= options[i].option;
        }
        else {
            m_option &= ~options[i].option;
        }
    }
#endif
}

gboolean
LibPinyinConfig::valueChanged (const std::string &section,
                               const std::string &name,
                               GVariant          *value)
{
    if (m_section != section)
        return FALSE;

    /* lookup table page size */
    if (CONFIG_ORIENTATION == name) {
        m_orientation = normalizeGVariant (value, IBUS_ORIENTATION_HORIZONTAL);
        if (m_orientation != IBUS_ORIENTATION_VERTICAL &&
            m_orientation != IBUS_ORIENTATION_HORIZONTAL) {
            m_orientation = IBUS_ORIENTATION_HORIZONTAL;
            g_warn_if_reached ();
        }
    }
    else if (CONFIG_PAGE_SIZE == name) {
        m_page_size = normalizeGVariant (value, 5);
        if (m_page_size > 10) {
            m_page_size = 5;
            g_warn_if_reached ();
        }
    } else if (CONFIG_REMEMBER_EVERY_INPUT == name) {
        m_remember_every_input = normalizeGVariant (value, false);
    } else if (CONFIG_DICTIONARIES == name) {
        m_dictionaries = normalizeGVariant (value, std::string (""));
    } else if (CONFIG_MAIN_SWITCH == name) {
        m_main_switch = normalizeGVariant (value, std::string ("<Shift>"));
    } else if (CONFIG_LETTER_SWITCH == name) {
        m_letter_switch = normalizeGVariant (value, std::string (""));
    } else if (CONFIG_PUNCT_SWITCH == name) {
        m_punct_switch = normalizeGVariant (value, std::string ("<Control>period"));
    } else if (CONFIG_TRAD_SWITCH == name) {
        m_trad_switch = normalizeGVariant (value, std::string ("<Control><Shift>f"));
    }
    /* fuzzy pinyin */
    else if (CONFIG_FUZZY_PINYIN == name) {
        if (normalizeGVariant (value, false))
            m_option_mask |= PINYIN_AMB_ALL;
        else
            m_option_mask &= ~PINYIN_AMB_ALL;
    }
    else {
        for (guint i = 0; i < G_N_ELEMENTS (options); i++) {
            if (G_LIKELY (options[i].name != name))
                continue;
            if (normalizeGVariant (value,
                    (options[i].option & PINYIN_DEFAULT_OPTION) != 0))
                m_option |= options[i].option;
            else
                m_option &= ~options[i].option;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void
LibPinyinConfig::valueChangedCallback (IBusConfig  *config,
                                       const gchar *section,
                                       const gchar *name,
                                       GVariant    *value,
                                       LibPinyinConfig *self)
{
    if (self->m_section != section)
        return;

    self->valueChanged (section, name, value);

    if (self->m_section == "engine/pinyin")
        LibPinyinBackEnd::instance ().setPinyinOptions (self);
    if (self->m_section == "engine/bopomofo")
        LibPinyinBackEnd::instance ().setChewingOptions (self);
}

static const struct {
    const gchar * const name;
    guint option;
} pinyin_options [] = {
    /* correct */
    { "correct-pinyin-gn-ng",    PINYIN_CORRECT_GN_NG    },
    { "correct-pinyin-gn-ng",    PINYIN_CORRECT_GN_NG    },
    { "correct-pinyin-mg-ng",    PINYIN_CORRECT_MG_NG    },
    { "correct-pinyin-iou-iu",   PINYIN_CORRECT_IOU_IU   },
    { "correct-pinyin-uei-ui",   PINYIN_CORRECT_UEI_UI   },
    { "correct-pinyin-uen-un",   PINYIN_CORRECT_UEN_UN   },
    { "correct-pinyin-ue-ve",    PINYIN_CORRECT_UE_VE    },
    { "correct-pinyin-v-u",      PINYIN_CORRECT_V_U      },
    { "correct-pinyin-ve-ue",    PINYIN_CORRECT_V_U      },
    { "correct-pinyin-on-ong",   PINYIN_CORRECT_ON_ONG   },
};

/* Here are the double pinyin keyboard scheme mapping table. */
static const struct{
    gint double_pinyin_keyboard;
    DoublePinyinScheme scheme;
} double_pinyin_schemes [] = {
    {0, DOUBLE_PINYIN_MS},
    {1, DOUBLE_PINYIN_ZRM},
    {2, DOUBLE_PINYIN_ABC},
    {3, DOUBLE_PINYIN_ZIGUANG},
    {4, DOUBLE_PINYIN_PYJJ},
    {5, DOUBLE_PINYIN_XHE}
};

PinyinConfig::PinyinConfig (Bus & bus)
    : LibPinyinConfig (bus, "libpinyin")
{
}

void
PinyinConfig::init (Bus & bus)
{
    if (m_instance.get () == NULL) {
        m_instance.reset (new PinyinConfig (bus));
        m_instance->readDefaultValues ();
    }
}

void
PinyinConfig::readDefaultValues (void)
{
    LibPinyinConfig::readDefaultValues ();
#if !defined(HAVE_IBUS_CONFIG_GET_VALUES)
    /* double pinyin */
    m_double_pinyin = read (CONFIG_DOUBLE_PINYIN, false);

    const gint map = read (CONFIG_DOUBLE_PINYIN_SCHEMA, 0);
    m_double_pinyin_schema = DOUBLE_PINYIN_DEFAULT;

    for (guint i = 0; i < G_N_ELEMENTS (double_pinyin_schemes); i++) {
        if (map == double_pinyin_schemes[i].double_pinyin_keyboard) {
            /* set double pinyin scheme. */
            m_double_pinyin_schema = double_pinyin_schemes[i].scheme;
        }
    }

    m_double_pinyin_show_raw = read (CONFIG_DOUBLE_PINYIN_SHOW_RAW, false);

    /* init states */
    m_init_chinese = read (CONFIG_INIT_CHINESE, true);
    m_init_full = read (CONFIG_INIT_FULL, false);
    m_init_full_punct = read (CONFIG_INIT_FULL_PUNCT, true);
    m_init_simp_chinese = read (CONFIG_INIT_SIMP_CHINESE, true);

    m_special_phrases = read (CONFIG_SPECIAL_PHRASES, true);

    /* other */
    m_shift_select_candidate = read (CONFIG_SHIFT_SELECT_CANDIDATE, false);
    m_minus_equal_page = read (CONFIG_MINUS_EQUAL_PAGE, true);
    m_comma_period_page = read (CONFIG_COMMA_PERIOD_PAGE, true);
    m_auto_commit = read (CONFIG_AUTO_COMMIT, false);

    /* correct pinyin */
    if (read (CONFIG_CORRECT_PINYIN, true))
        m_option_mask |= PINYIN_CORRECT_ALL;
    else
        m_option_mask &= ~PINYIN_CORRECT_ALL;

    /* read values */
    for (guint i = 0; i < G_N_ELEMENTS (pinyin_options); i++) {
        if (read (pinyin_options[i].name,
                (pinyin_options[i].option & PINYIN_DEFAULT_OPTION) != 0))
            m_option |= pinyin_options[i].option;
        else
            m_option &= ~pinyin_options[i].option;
    }
#endif
}

gboolean
PinyinConfig::valueChanged (const std::string &section,
                                     const std::string &name,
                                     GVariant          *value)
{
    if (m_section != section)
        return FALSE;

    if (LibPinyinConfig::valueChanged (section, name, value))
        return TRUE;

    /* double pinyin */
    if (CONFIG_DOUBLE_PINYIN == name)
        m_double_pinyin = normalizeGVariant (value, false);
    else if (CONFIG_DOUBLE_PINYIN_SCHEMA == name) {
        const gint map = normalizeGVariant (value, 0);
        m_double_pinyin_schema = DOUBLE_PINYIN_DEFAULT;

        for (guint i = 0; i < G_N_ELEMENTS (double_pinyin_schemes); i++) {
            if (map == double_pinyin_schemes[i].double_pinyin_keyboard) {
                /* set double pinyin scheme. */
                m_double_pinyin_schema = double_pinyin_schemes[i].scheme;
            }
        }
    }
    else if (CONFIG_DOUBLE_PINYIN_SHOW_RAW == name)
        m_double_pinyin_show_raw = normalizeGVariant (value, false);
    /* init states */
    else if (CONFIG_INIT_CHINESE == name)
        m_init_chinese = normalizeGVariant (value, true);
    else if (CONFIG_INIT_FULL == name)
        m_init_full = normalizeGVariant (value, true);
    else if (CONFIG_INIT_FULL_PUNCT == name)
        m_init_full_punct = normalizeGVariant (value, true);
    else if (CONFIG_INIT_SIMP_CHINESE == name)
        m_init_simp_chinese = normalizeGVariant (value, true);
    else if (CONFIG_SPECIAL_PHRASES == name)
        m_special_phrases = normalizeGVariant (value, true);
    /* others */
    else if (CONFIG_SHIFT_SELECT_CANDIDATE == name)
        m_shift_select_candidate = normalizeGVariant (value, false);
    else if (CONFIG_MINUS_EQUAL_PAGE == name)
        m_minus_equal_page = normalizeGVariant (value, true);
    else if (CONFIG_COMMA_PERIOD_PAGE == name)
        m_comma_period_page = normalizeGVariant (value, true);
    else if (CONFIG_AUTO_COMMIT == name)
        m_auto_commit = normalizeGVariant (value, false);
    else if (CONFIG_IMPORT_DICTIONARY == name) {
        std::string filename = normalizeGVariant (value, std::string(""));
        LibPinyinBackEnd::instance ().importPinyinDictionary (filename.c_str ());
    }
    else if (CONFIG_EXPORT_DICTIONARY == name) {
        std::string filename = normalizeGVariant (value, std::string(""));
        LibPinyinBackEnd::instance ().exportPinyinDictionary (filename.c_str ());
    }
    else if (CONFIG_CLEAR_USER_DATA == name) {
        std::string target = normalizeGVariant (value, std::string(""));
        LibPinyinBackEnd::instance ().clearPinyinUserData(target.c_str ());
    } /* correct pinyin */
    else if (CONFIG_CORRECT_PINYIN == name) {
        if (normalizeGVariant (value, true))
            m_option_mask |= PINYIN_CORRECT_ALL;
        else
            m_option_mask &= ~PINYIN_CORRECT_ALL;
    }
    else {
        for (guint i = 0; i < G_N_ELEMENTS (pinyin_options); i++) {
            if (G_LIKELY (pinyin_options[i].name != name))
                continue;
            if (normalizeGVariant (value,
                    (pinyin_options[i].option & PINYIN_DEFAULT_OPTION) != 0))
                m_option |= pinyin_options[i].option;
            else
                m_option &= ~pinyin_options[i].option;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

/* Here are the chewing keyboard scheme mapping table. */
static const struct {
    gint bopomofo_keyboard;
    ZhuyinScheme scheme;
} chewing_schemes [] = {
    {0, ZHUYIN_STANDARD},
    {1, ZHUYIN_GINYIEH},
    {2, ZHUYIN_ETEN},
    {3, ZHUYIN_IBM}
};

BopomofoConfig::BopomofoConfig (Bus & bus)
    : LibPinyinConfig (bus, "libbopomofo")
{
}

void
BopomofoConfig::init (Bus & bus)
{
    if (m_instance.get () == NULL) {
        m_instance.reset (new BopomofoConfig (bus));
        m_instance->readDefaultValues ();
    }
}

void
BopomofoConfig::readDefaultValues (void)
{
    LibPinyinConfig::readDefaultValues ();
#if !defined(HAVE_IBUS_CONFIG_GET_VALUES)
    /* init states */
    m_init_chinese = read (CONFIG_INIT_CHINESE, true);
    m_init_full = read (CONFIG_INIT_FULL, false);
    m_init_full_punct = read (CONFIG_INIT_FULL_PUNCT, true);
    m_init_simp_chinese = read (CONFIG_INIT_SIMP_CHINESE, false);

    m_special_phrases = read (CONFIG_SPECIAL_PHRASES, false);

    const gint map = read (CONFIG_BOPOMOFO_KEYBOARD_MAPPING, 0);
    m_bopomofo_keyboard_mapping = ZHUYIN_DEFAULT;

    for (guint i = 0; i < G_N_ELEMENTS (chewing_schemes); i++) {
        if (map == chewing_schemes[i].bopomofo_keyboard) {
            /* set chewing scheme. */
            m_bopomofo_keyboard_mapping = chewing_schemes[i].scheme;
        }
    }

    m_select_keys = read (CONFIG_SELECT_KEYS, 0);
    if (m_select_keys >= 9) m_select_keys = 0;
    m_guide_key = read (CONFIG_GUIDE_KEY, true);
    m_auxiliary_select_key_f = read (CONFIG_AUXILIARY_SELECT_KEY_F, true);
    m_auxiliary_select_key_kp = read (CONFIG_AUXILIARY_SELECT_KEY_KP, true);
    m_enter_key = read (CONFIG_ENTER_KEY, true);
#endif
}

gboolean
BopomofoConfig::valueChanged (const std::string &section,
                                       const std::string &name,
                                       GVariant          *value)
{
    if (m_section != section)
        return FALSE;

    if (LibPinyinConfig::valueChanged (section, name, value))
        return TRUE;

    /* init states */
    if (CONFIG_INIT_CHINESE == name)
        m_init_chinese = normalizeGVariant (value, true);
    else if (CONFIG_INIT_FULL == name)
        m_init_full = normalizeGVariant (value, true);
    else if (CONFIG_INIT_FULL_PUNCT == name)
        m_init_full_punct = normalizeGVariant (value, true);
    else if (CONFIG_INIT_SIMP_CHINESE == name)
        m_init_simp_chinese = normalizeGVariant (value, false);
    else if (CONFIG_SPECIAL_PHRASES == name)
        m_special_phrases = normalizeGVariant (value, false);
    else if (CONFIG_BOPOMOFO_KEYBOARD_MAPPING == name) {
        const gint map = normalizeGVariant (value, 0);
        m_bopomofo_keyboard_mapping = ZHUYIN_DEFAULT;

        for (guint i = 0; i < G_N_ELEMENTS (chewing_schemes); i++) {
            if (map == chewing_schemes[i].bopomofo_keyboard) {
                /* set chewing scheme. */
                m_bopomofo_keyboard_mapping = chewing_schemes[i].scheme;
            }
        }
    } else if (CONFIG_SELECT_KEYS == name) {
        m_select_keys = normalizeGVariant (value, 0);
        if (m_select_keys >= 9) m_select_keys = 0;
    }
    else if (CONFIG_GUIDE_KEY == name)
        m_guide_key = normalizeGVariant (value, true);
    else if (CONFIG_AUXILIARY_SELECT_KEY_F == name)
        m_auxiliary_select_key_f = normalizeGVariant (value, true);
    else if (CONFIG_AUXILIARY_SELECT_KEY_KP == name)
        m_auxiliary_select_key_kp = normalizeGVariant (value, true);
    else if (CONFIG_ENTER_KEY == name)
        m_enter_key = normalizeGVariant (value, true);
    else
        return FALSE;
    return TRUE;

}

};
