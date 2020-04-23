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
#ifndef __PY_EXT_EDITOR_
#define __PY_EXT_EDITOR_

#include "lua-plugin.h"

namespace PY {

class ExtEditor : public Editor {
public:
    ExtEditor (PinyinProperties & props, Config & config);

    virtual gboolean processKeyEvent (guint keyval, guint keycode, guint modifiers);
    virtual void pageUp (void);
    virtual void pageDown (void);
    virtual void cursorUp (void);
    virtual void cursorDown (void);
    virtual void update (void);
    virtual void reset (void);
    virtual void candidateClicked (guint index, guint button, guint state);

    gboolean setLuaPlugin (IBusEnginePlugin *plugin);

private:
    bool updateStateFromInput (void);

    /* Fill lookup table, and update preedit string. */
    bool fillCommandCandidates (void);
    bool fillCommandCandidates (std::string prefix);
    bool fillCommand (std::string command_name, const char * argument);

    bool fillChineseNumber(gint64 num);

    /* Auxiliary functions for lookup table */
    void clearLookupTable (void);
    void updateLookupTable (void);
    gboolean selectCandidateInPage (guint index);
    gboolean selectCandidate (guint index);

    void updatePreeditText (void);
    void updateAuxiliaryText (void);

    gboolean processEditKey (guint keyval);
    gboolean processPageKey (guint keyval);
    gboolean processLabelKey (guint keyval);

    gboolean processSpace (guint keyval);
    gboolean processEnter (guint keyval);

    gboolean removeCharBefore (void);
    gboolean removeCharAfter (void);

    enum LabelMode{
        LABEL_NONE,
        LABEL_LIST_NUMBERS,
        LABEL_LIST_COMMANDS,
        LABEL_LIST_NONE,
        LABEL_LIST_DIGIT,
        LABEL_LIST_ALPHA,
        LABEL_LIST_SINGLE,
        LABEL_LAST,
    };
    LabelMode m_mode;
    Pointer<IBusEnginePlugin> m_lua_plugin;

    String m_preedit_text;
    String m_auxiliary_text;

    LookupTable m_lookup_table;

    //saved lua extension call results.
    int m_result_num;
    const lua_command_candidate_t * m_candidate;
    GArray * m_candidates;

    const static int m_aux_text_len = 50;
};

};
#endif
