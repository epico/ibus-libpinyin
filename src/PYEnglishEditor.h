/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2010-2011 Peng Wu <alexepico@gmail.com>
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

#ifndef __PY_ENGLISH_EDITOR_
#define __PY_ENGLISH_EDITOR_

#include "PYEditor.h"
#include "PYLookupTable.h"
#include "PYEnglishDatabase.h"

namespace PY {

class EnglishDatabase;

static const std::string EnglishSymbols = "`~!@*()+{}\\|:\"/<>?";

class EnglishEditor : public Editor {
private:
    const float m_train_factor;
public:
    EnglishEditor (PinyinProperties &props, Config & config);
    virtual ~EnglishEditor();

    virtual gboolean processKeyEvent (guint keyval, guint keycode, guint modifers);
    virtual void pageUp (void);
    virtual void pageDown (void);
    virtual void cursorUp (void);
    virtual void cursorDown (void);
    virtual void update (void);
    virtual void updateAll (void);
    virtual void reset (void);
    virtual void candidateClicked (guint index, guint button, guint state);

private:
    gboolean updateStateFromInput (void);

    void clearLookupTable (void);
    void updateLookupTable (void);
    void updatePreeditText (void);
    void updateAuxiliaryText (void);

    gboolean selectCandidateInPage (guint index);
    gboolean selectCandidate (guint index);

    gboolean processSpace (guint keyval);
    gboolean processEnter (guint keyval);

    gboolean removeCharBefore (void);
    gboolean removeCharAfter (void);

    gboolean processLabelKey(guint keyval);
    gboolean processEditKey(guint keyval);
    gboolean processPageKey(guint keyval);

private:
    /* variables */
    LookupTable m_lookup_table;

    String m_preedit_text;
    String m_auxiliary_text;

    EnglishDatabase *m_english_database;

    const static int m_aux_text_len = 50;
};

};

#endif
