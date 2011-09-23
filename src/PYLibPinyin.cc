/* vim:set et ts=4 sts=4:
 *
 * ibus-pinyin - The Chinese PinYin engine for IBus
 *
 * Copyright (c) 2011 Peng Wu <alexepico@gmail.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "PYLibPinyin.h"
#include "PYTypes.h"
#include "PYConfig.h"

using namespace PY;

std::unique_ptr<LibPinyinBackEnd> LibPinyinBackEnd::m_instance;

static LibPinyinBackEnd libpinyin_backend;

LibPinyinBackEnd::LibPinyinBackEnd(){
    g_assert (NULL == m_instance.get ());
    m_pinyin_context = pinyin_init("/usr/share/libpinyin/data", "../data");
    m_instance.reset(this);
}

LibPinyinBackEnd::~LibPinyinBackEnd(){
    pinyin_fini(m_pinyin_context);
    m_instance = NULL;
}

/* Here are the fuzzy pinyin options conversion table. */
static const struct {
    guint ibus_pinyin_option;
    PinyinAmbiguity libpinyin_option;
} fuzzy_options [] = {
    /* fuzzy pinyin */
    { PINYIN_FUZZY_C_CH,        PINYIN_AmbCiChi        },
    { PINYIN_FUZZY_CH_C,        PINYIN_AmbChiCi        },
    { PINYIN_FUZZY_Z_ZH,        PINYIN_AmbZiZhi        },
    { PINYIN_FUZZY_ZH_Z,        PINYIN_AmbZhiZi        },
    { PINYIN_FUZZY_S_SH,        PINYIN_AmbSiShi        },
    { PINYIN_FUZZY_SH_S,        PINYIN_AmbShiSi        },
    { PINYIN_FUZZY_L_N,         PINYIN_AmbLeNe         },
    { PINYIN_FUZZY_N_L,         PINYIN_AmbNeLe         },
    { PINYIN_FUZZY_F_H,         PINYIN_AmbFoHe         },
    { PINYIN_FUZZY_H_F,         PINYIN_AmbHeFo         },
    { PINYIN_FUZZY_L_R,         PINYIN_AmbLeRi         },
    { PINYIN_FUZZY_R_L,         PINYIN_AmbRiLe         },
    { PINYIN_FUZZY_K_G,         PINYIN_AmbKeGe         },
    { PINYIN_FUZZY_G_K,         PINYIN_AmbGeKe         },
    { PINYIN_FUZZY_AN_ANG,      PINYIN_AmbAnAng        },
    { PINYIN_FUZZY_ANG_AN,      PINYIN_AmbAngAn        },
    { PINYIN_FUZZY_EN_ENG,      PINYIN_AmbEnEng        },
    { PINYIN_FUZZY_ENG_EN,      PINYIN_AmbEngEn        },
    { PINYIN_FUZZY_IN_ING,      PINYIN_AmbInIng        },
    { PINYIN_FUZZY_ING_IN,      PINYIN_AmbIngIn        }
};


gboolean
LibPinyinBackEnd::setPinyinOptions (Config * config)
{
    guint option = config->option ();
    PinyinCustomSettings custom;

    custom.set_use_incomplete (option & PINYIN_INCOMPLETE_PINYIN);
    custom.set_use_ambiguities (PINYIN_AmbAny, false);

    /* copy values */
    for (guint i = 0; i < G_N_ELEMENTS (fuzzy_options); i++) {
        if ( option & fuzzy_options[i].ibus_pinyin_option )
            custom.set_use_ambiguities
                (fuzzy_options[i].libpinyin_option, true);
    }

    pinyin_set_options(m_pinyin_context, &custom);

    return TRUE;
}

/* Here are the chewing keyboard scheme mapping table. */
static const struct {
    guint bopomofo_keyboard;
    PinyinZhuYinScheme chewing_keyboard;
} chewing_options [] = {
    {0, ZHUYIN_STANDARD},
    {1, ZHUYIN_GIN_YIEH},
    {2, ZHUYIN_ET26},
    {3, ZHUYIN_IBM}
};


gboolean
LibPinyinBackEnd::setChewingOptions (Config *config)
{
    const guint map = config->bopomofoKeyboardMapping ();
    for (guint i = 0; i < G_N_ELEMENTS (chewing_options); i++) {
        if (map == chewing_options[i].bopomofo_keyboard){
            /* TODO: set chewing scheme. */
            PinyinZhuYinScheme scheme = chewing_options[i].chewing_keyboard;
            g_assert (FALSE);
        }            
    }
    return TRUE;
}
