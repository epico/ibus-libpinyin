/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2011 Peng Wu <alexepico@gmail.com>
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
#ifndef __PY_LIB_PINYIN_FULL_PINYIN_EDITOR_H
#define __PY_LIB_PINYIN_FULL_PINYIN_EDITOR_H

#include "PYPPinyinEditor.h"

namespace PY {

class FullPinyinEditor : public PinyinEditor {

public:
    FullPinyinEditor (PinyinProperties & props, Config & config);
    ~FullPinyinEditor (void);

public:
    gboolean insert (gint ch);

    /* virtual functions */
    virtual gboolean processKeyEvent (guint keyval, guint keycode, guint modifiers);
    virtual void reset (void);
    virtual void updateAuxiliaryText (void);

protected:

    virtual void updatePinyin (void);

    virtual guint getLookupCursor (void);

};

};

#endif
