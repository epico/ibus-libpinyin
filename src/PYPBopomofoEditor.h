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
#ifndef __PY_LIB_PINYIN_BOPOMOFO_EDITOR_H_
#define __PY_LIB_PINYIN_BOPOMOFO_EDITOR_H_

#include "PYPPhoneticEditor.h"

namespace PY {

class Config;

#define MAX_PINYIN_LEN 64

class BopomofoEditor : public PhoneticEditor {

public:
    BopomofoEditor (PinyinProperties & props, Config & config);
    virtual ~BopomofoEditor (void);

protected:
    String bopomofo;
    gboolean m_select_mode;

    gboolean processGuideKey (guint keyval, guint keycode, guint modifiers);
    gboolean processAuxiliarySelectKey (guint keyval, guint keycode,
                                        guint modifiers);
    gboolean processSelectKey (guint keyval, guint keycode, guint modifiers);
    gboolean processBopomofo (guint keyval, guint keycode, guint modifiers);
    gboolean processKeyEvent (guint keyval, guint keycode, guint modifiers);

    void updateLookupTableLabel ();
    virtual void updateLookupTable ();

    virtual void updatePreeditText ();
    virtual void updateAuxiliaryText ();
    virtual void updatePinyin (void);
    virtual void commit (const gchar *str);
    using PhoneticEditor::commit;

    void reset ();

    gboolean insert (gint ch);

};

};


#endif
