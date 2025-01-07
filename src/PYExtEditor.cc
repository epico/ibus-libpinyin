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

#include <cstring>
#include <string>

#include "PYConfig.h"
#include "PYPointer.h"
#include "PYLookupTable.h"

#include "PYEditor.h"
#include "PYExtEditor.h"

namespace PY {


static const char * numbers [2][10] = {
    {"零", "壹", "贰", "叁", "肆", "伍", "陆", "柒", "捌", "玖",},
    {"〇", "一", "二", "三", "四", "五", "六", "七", "八", "九",},
};

struct unit_t{
    const char * unit_zh_name;  // Chinese Character
    const int digits;           // Position in string.
    const bool persist;         // Whether to force eating zero and force inserting into result string.
};

static unit_t units_simplified[] ={
    {"兆", 12, true},
    {"亿", 8, true},
    {"万", 4, true},
    {"千", 3, false},
    {"百", 2, false},
    {"十", 1, false},
    {"",   0, true},
};

static unit_t units_traditional[] ={
    {"兆", 12, true},
    {"亿", 8, true},
    {"万", 4, true},
    {"仟", 3, false},
    {"佰", 2, false},
    {"拾", 1, false},
    {"",   0, true},
};


static const std::string
simplest_cn_number(gint64 num)
{
    std::string result = "";
    if ( num == 0 )
        result = numbers[1][0];
    while (num > 0) {
        int remains = num % 10;
        num = num / 10;
        result = std::string ( numbers[1][remains] ) + result;
    }

    return result;
}

static inline const std::string
translate_to_longform(gint64 num, const char * number[10], unit_t units[], int units_len)
{
    std::string result = "";
    int cur_pos = -1;
    bool eat_zero = false;

    while (num > 0) {
        int remains = num % 10;
        num = num / 10;
        cur_pos ++;
        std::string unit = "";
        int pos = cur_pos;
        size_t i = 6;
        while ( pos > 0 ) {
            for ( i = 0; i < units_len; ++i) {
                pos = pos % units[i].digits;
                if ( pos == 0 )
                    break;
            }
        }

        if ( units[i].persist ) {
            result = std::string (units[i].unit_zh_name) + result;
            eat_zero = true;
        }

        if ( remains == 0){
            if ( eat_zero ) continue;

            result = std::string (number[0]) + result;
            eat_zero = true;
            continue;
        }else{
            eat_zero = false;
        }

        if (num == 0 && remains == 1 && i == 5)
            result = std::string (units[i].unit_zh_name) + result;
        else if (units[i].persist)
            result = std::string (number[remains]) + result;
        else
            result = std::string (number[remains]) + std::string (units[i].unit_zh_name) + result;
    }

    return result;
}

static const std::string
simplified_number(gint64 num)
{
    return translate_to_longform(num, numbers[1], units_simplified, G_N_ELEMENTS(units_simplified));
}

static const std::string
traditional_number(gint64 num)
{
    if ( 0 == num )
        return numbers[0][0];
    return translate_to_longform(num, numbers[0], units_traditional, G_N_ELEMENTS(units_traditional));
}



/* Write digit/alpha/none Label generator here.
 * foreach (results): 1, from get_retval; 2..n from get_retvals.
 */

ExtEditor::ExtEditor (PinyinProperties & props, Config & config)
    : Editor (props, config),
      m_mode (LABEL_NONE),
      m_result_num (0),
      m_candidate (NULL),
      m_candidates (NULL)
{
}

gboolean
ExtEditor::setLuaPlugin (IBusEnginePlugin *plugin)
{
    m_lua_plugin = plugin;
    return TRUE;
}

gboolean
ExtEditor::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
    if (modifiers & IBUS_MOD4_MASK)
        return FALSE;

    //IBUS_SHIFT_MASK is removed.
    modifiers &= (IBUS_CONTROL_MASK |
                  IBUS_MOD1_MASK |
                  IBUS_SUPER_MASK |
                  IBUS_HYPER_MASK |
                  IBUS_META_MASK |
                  IBUS_LOCK_MASK);
    if ( modifiers )
        return FALSE;

    //handle backspace/delete here.
    if (processEditKey (keyval))
        return TRUE;

    //handle page/cursor up/down here.
    if (processPageKey (keyval))
        return TRUE;

    //handle label key select here.
    if (processLabelKey (keyval))
        return TRUE;

    if (processSpace (keyval))
        return TRUE;

    if (processEnter (keyval))
        return TRUE;

    m_cursor = std::min (m_cursor, (guint)m_text.length ());

    /* Remember the input string. */
    switch (m_cursor) {
    case 0: //Empty input string.
        {
            g_return_val_if_fail ( 'i' == keyval || 'I' == keyval, FALSE);
            if ( 'i' == keyval || 'I' == keyval) {
                m_text.insert (m_cursor, keyval);
                m_cursor++;
            }
        }
        break;
    case 1 ... 2: // Only contains 'i' or 'I' in input string.
        {
            g_return_val_if_fail ( 'i' == m_text[0] || 'I' == m_text[0], FALSE);
            if ( isalnum (keyval) ) {
                m_text.insert (m_cursor, keyval);
                m_cursor++;
            }
        }
        break;
    default: //Here is the appended argment.
        {
            g_return_val_if_fail ( 'i' == m_text[0] || 'I' == m_text[0], FALSE);
            if (isprint (keyval)) {
                m_text.insert (m_cursor, keyval);
                m_cursor++;
            }
        }
        break;
    }
    /* Deal other staff with updateStateFromInput (). */
    updateStateFromInput ();
    update ();
    return TRUE;
}

gboolean
ExtEditor::processEditKey (guint keyval)
{
    switch (keyval) {
    case IBUS_Delete:
    case IBUS_KP_Delete:
        removeCharAfter ();
        updateStateFromInput ();
        update ();
        return TRUE;
    case IBUS_BackSpace:
        removeCharBefore ();
        updateStateFromInput ();
        update ();
        return TRUE;
    }
    return FALSE;
}

gboolean
ExtEditor::processPageKey (guint keyval)
{
    switch (keyval) {
    //For "2000-10-10" and "16:30" input.
    case IBUS_comma:
        if (m_config.commaPeriodPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
#if 0
    case IBUS_minus:
        if (m_config.minusEqualPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
#endif
    case IBUS_bracketleft:
        if (m_config.squareBracketPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
#if 0
    //For "2.5" input.
    case IBUS_period:
        if (m_config.commaPeriodPage ()) {
            pageDown ();
            return TRUE;
        }
        break;
#endif
    case IBUS_equal:
        if (m_config.minusEqualPage ()) {
            pageDown ();
            return TRUE;
        }
        break;
    case IBUS_bracketright:
        if (m_config.squareBracketPage ()) {
            pageDown ();
            return TRUE;
        }
        break;

    case IBUS_Up:
    case IBUS_KP_Up:
        cursorUp ();
        return TRUE;

    case IBUS_Down:
    case IBUS_KP_Down:
        cursorDown ();
        return TRUE;

    case IBUS_Page_Up:
    case IBUS_KP_Page_Up:
        pageUp ();
        return TRUE;

    case IBUS_Page_Down:
    case IBUS_KP_Page_Down:
        pageDown ();
        return TRUE;

    case IBUS_Escape:
        reset ();
        return TRUE;
    }
    return FALSE;
}

gboolean
ExtEditor::processLabelKey (guint keyval)
{
    //According to enum ExtEditorLabelMode.

    switch (m_mode) {
    case LABEL_LIST_DIGIT:
        switch (keyval) {
        case '1' ... '9':
            return selectCandidateInPage (keyval - '1');
            break;
        case '0':
            return selectCandidateInPage (9);
            break;
        }
        break;
    case LABEL_LIST_NUMBERS:
    case LABEL_LIST_ALPHA:
        switch (keyval) {
        case 'a' ... 'k':
            return selectCandidateInPage (keyval - 'a');
            break;
        case 'A' ... 'K':
            return selectCandidateInPage (keyval - 'A');
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

gboolean
ExtEditor::processSpace (guint keyval)
{
    if (!(keyval == IBUS_space || keyval == IBUS_KP_Space))
        return FALSE;

    guint cursor_pos = m_lookup_table.cursorPos ();

    switch (m_mode) {
    case LABEL_LIST_NUMBERS:
        selectCandidate (cursor_pos);
        break;
    case LABEL_LIST_COMMANDS:
    case LABEL_LIST_DIGIT:
    case LABEL_LIST_ALPHA:
    case LABEL_LIST_NONE:
        selectCandidate (cursor_pos);
        break;
    case LABEL_LIST_SINGLE:
        g_return_val_if_fail (cursor_pos == 0 , FALSE);
        selectCandidate (cursor_pos);
        break;
    default:
        break;
    }
    return TRUE;
}

gboolean
ExtEditor::processEnter(guint keyval)
{
    if (keyval != IBUS_Return)
        return FALSE;

    if (m_text.length () == 0)
        return FALSE;

    Text text(m_text);
    commitText (text);
    reset ();
    return TRUE;
}

void
ExtEditor::pageUp (void)
{
    if (G_LIKELY(m_lookup_table.pageUp ())) {
        update ();
    }
}

void
ExtEditor::pageDown (void)
{
    if (G_LIKELY(m_lookup_table.pageDown ())) {
        update ();
    }
}

gboolean
ExtEditor::removeCharBefore (void)
{
    if (G_UNLIKELY( m_cursor <= 0 )) {
        m_cursor = 0;
        return FALSE;
    }

    if (G_UNLIKELY( m_cursor > m_text.length () )) {
        m_cursor = m_text.length ();
        return FALSE;
    }

    m_text.erase (m_cursor - 1, 1);
    m_cursor = std::max (0, static_cast<int>(m_cursor) - 1);
    return TRUE;
}

gboolean
ExtEditor::removeCharAfter (void)
{
    if (G_UNLIKELY( m_cursor < 0 )) {
        m_cursor = 0;
        return FALSE;
    }

    if (G_UNLIKELY( m_cursor >= m_text.length () )) {
        m_cursor = m_text.length ();
        return FALSE;
    }
    m_text.erase (m_cursor, 1);
    m_cursor = std::min (m_cursor, (guint)m_text.length ());
    return TRUE;
}

void
ExtEditor::cursorUp (void)
{
    if (G_LIKELY (m_lookup_table.cursorUp ())) {
        update ();
    }
}

void
ExtEditor::cursorDown (void)
{
    if (G_LIKELY (m_lookup_table.cursorDown ())) {
        update ();
    }
}

void
ExtEditor::update (void)
{
    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}

void
ExtEditor::updateAll (void)
{
    updateStateFromInput ();
    update ();
}

void
ExtEditor::reset (void)
{
    m_text = "";
    updateStateFromInput ();
    update ();
}

void
ExtEditor::candidateClicked (guint index, guint button, guint state)
{
    selectCandidateInPage (index);
}

gboolean
ExtEditor::selectCandidateInPage (guint index)
{
    guint page_size = m_lookup_table.pageSize ();
    guint cursor_pos = m_lookup_table.cursorPos ();

    if (G_UNLIKELY(index >= page_size))
        return FALSE;
    index += (cursor_pos / page_size) * page_size;

    return selectCandidate (index);
}

gboolean
ExtEditor::selectCandidate (guint index)
{
    switch (m_mode) {
    case LABEL_LIST_NUMBERS:
        {
            if ( index >= m_lookup_table.size() )
                return FALSE;

            IBusText * candidate = m_lookup_table.getCandidate(index);
            Text text(candidate);
            commitText(text);
            reset();
            return TRUE;
        }
        break;
    case LABEL_LIST_COMMANDS:
        {
            std::string prefix = m_text.substr (1, 2);
            int len = prefix.length ();
            const char * prefix_str = prefix.c_str ();
            const GArray * commands = ibus_engine_plugin_get_available_commands (m_lua_plugin);
            int match_count = -1;
            for (int i = 0; i < static_cast<int>(commands->len); ++i) {
                lua_command_t * command = &g_array_index (commands, lua_command_t, i);
                if ( strncmp (prefix_str, command->command_name, len) == 0 ) {
                    match_count++;
                }
                if ( match_count == static_cast<int>(index) ) {
                    m_text.clear ();
                    m_text = "i";
                    m_text += command->command_name;
                    m_cursor = m_text.length ();
                    break;
                }
            }
            updateStateFromInput ();
            update ();
        }
        return TRUE;
        break;
    case LABEL_LIST_DIGIT:
    case LABEL_LIST_ALPHA:
    case LABEL_LIST_NONE:
        {
            g_return_val_if_fail (m_result_num > 1, FALSE);
            g_return_val_if_fail (static_cast<int>(index) < m_result_num, FALSE);

            const lua_command_candidate_t * candidate = g_array_index (m_candidates, lua_command_candidate_t *, index);
            if ( candidate->content ) {
                Text text (candidate->content);
                commitText (text);
                m_text.clear ();
            } else if (candidate->suggest) {
                m_text += candidate->suggest;
                m_cursor += strlen(candidate->suggest);
            }

            updateStateFromInput ();
            update ();
        }
        return TRUE;
        break;
    case LABEL_LIST_SINGLE:
        {
            g_return_val_if_fail (m_result_num == 1, FALSE);
            g_return_val_if_fail (index == 0, FALSE);
            if ( m_candidate->content ) {
                Text text (m_candidate->content);
                commitText (text);
                m_text.clear ();
            } else if (m_candidate->suggest) {
                m_text += m_candidate->suggest;
            }

            updateStateFromInput ();
            update ();
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

bool
ExtEditor::updateStateFromInput (void)
{
    /* Do parse and candidates update here. */
    /* prefix i double check here. */
    if ( !m_text.length () ) {
        m_preedit_text = "";
        m_auxiliary_text = "";
        m_cursor = 0;
        clearLookupTable ();
        return FALSE;
    }

    if ( 'i' != m_text[0] && 'I' != m_text[0] ) {
        g_warning ("'i' or 'I' is expected in m_text string.\n");
        return FALSE;
    }

    m_auxiliary_text = m_text[0];

    m_mode = LABEL_LIST_COMMANDS;
    if ( 1 == m_text.length () ) {
        fillCommandCandidates ();
        return true;
    }
    /* Check m_text len, and update auxiliary string meanwhile.
     * 1. only "i", dispatch to fillCommandCandidates (void).
     * 2. "i" with one charactor,
     *      dispatch to fillCommandCandidates (std::string).
     * 3. "i" with two charactor or more,
     *      dispatch to fillCommand (std::string, const char * argument).
     */

    if ( isalpha (m_text[1])) {
        m_mode = LABEL_LIST_COMMANDS;
        if ( m_text.length () == 2) {
            fillCommandCandidates (m_text.substr (1,1).c_str ());

            m_auxiliary_text += " ";
            m_auxiliary_text += m_text.substr (1, 1);
            return true;
        } else if ( m_text.length () >= 3) {
            std::string command_name = m_text.substr (1,2);

            m_auxiliary_text += " ";
            m_auxiliary_text += m_text.substr (1,2);

            const char * argment = NULL;
            std::string arg = "";
            if (m_text.length () > 3) {
                arg = m_text.substr (3);
                argment = arg.c_str ();
                m_auxiliary_text += " ";
                m_auxiliary_text += argment;
            }
            /* finish auxiliary text computing here. */

            const lua_command_t * command = ibus_engine_plugin_lookup_command (m_lua_plugin, command_name.c_str ());
            if ( NULL == command) {
                m_mode = LABEL_NONE;
                clearLookupTable ();
                m_lookup_table.clear ();
                return FALSE;
            }

            if ( command->help ){
                int space_len = std::max ( 0, m_aux_text_len
                                           - (int) g_utf8_strlen (command->help, -1)
                                           - 2 /* length of "[...]" */);
                m_auxiliary_text.append(space_len, ' ');

                m_auxiliary_text += "[";
                m_auxiliary_text += command->help;
                m_auxiliary_text += "]";
            }

            std::string label = command->leading;

            if ( "digit" == label )
                m_mode = LABEL_LIST_DIGIT;
            else if ( "alpha" == label )
                m_mode = LABEL_LIST_ALPHA;
            else
                m_mode = LABEL_LIST_NONE;

            fillCommand (command_name, argment);
        }
    }
    else if ( isdigit (m_text[1]) ) {
        m_mode = LABEL_LIST_NUMBERS;
        std::string number = m_text.substr(1);
        m_auxiliary_text += " ";
        m_auxiliary_text += number;

        //Generate Chinese number.
        gint64 num = atoll (number.c_str ());
        fillChineseNumber (num);
    }

    return true;
}

bool
ExtEditor::fillCommandCandidates (void)
{
    return fillCommandCandidates ("");
}

bool
ExtEditor::fillCommandCandidates (std::string prefix)
{
    clearLookupTable ();

    /* fill candidates here. */
    int len = prefix.length ();
    const char * prefix_str = prefix.c_str ();
    const GArray * commands = ibus_engine_plugin_get_available_commands (m_lua_plugin);
    int count = -1;
    for ( int i = 0; i < static_cast<int>(commands->len); ++i) {
        lua_command_t * command = &g_array_index (commands, lua_command_t, i);
        if ( strncmp (prefix_str, command->command_name, len) == 0) {
            count++;
            std::string candidate = command->command_name;
            candidate += ".";
            candidate += command->description;
            m_lookup_table.setLabel (count, Text (""));
            m_lookup_table.appendCandidate (Text (candidate));
        }
    }

    return true;
}

bool
ExtEditor::fillCommand (std::string command_name, const char * argument)
{
    const lua_command_t * command = ibus_engine_plugin_lookup_command (m_lua_plugin, command_name.c_str ());
    if ( NULL == command )
        return false;

    if ( m_result_num != 0) {
        if ( m_result_num == 1) {
            ibus_engine_plugin_free_candidate ((lua_command_candidate_t *)m_candidate);
            m_candidate = NULL;
        }else{
            for ( int i = 0; i < m_result_num; ++i) {
                const lua_command_candidate_t * candidate = g_array_index (m_candidates, lua_command_candidate_t *, i);
                ibus_engine_plugin_free_candidate ((lua_command_candidate_t *)candidate);
            }

            g_array_free (m_candidates, TRUE);
            m_candidates = NULL;
        }
        m_result_num = 0;
        g_assert (m_candidates == NULL && m_candidate == NULL);
    }

    m_result_num = ibus_engine_plugin_call (m_lua_plugin, command->lua_function_name, argument);

    if ( 1 == m_result_num )
        m_mode = LABEL_LIST_SINGLE;

    clearLookupTable ();

    //Generate labels according to m_mode
    if ( LABEL_LIST_DIGIT == m_mode ) {
        for ( int i = 1; i <= 10; ++i )
            m_lookup_table.setLabel ( i - 1, Text (i - 1 + '1') );
    }

    if ( LABEL_LIST_ALPHA == m_mode) {
        for ( int i = 1; i <= 10; ++i )
            m_lookup_table.setLabel ( i - 1, Text (i - 1 + 'a') );
    }

    if ( LABEL_LIST_NONE == m_mode || LABEL_LIST_SINGLE == m_mode) {
        for ( int i = 1; i <= 10; ++i)
            m_lookup_table.setLabel ( i - 1, Text (""));
    }

    //Generate candidates
    std::string result;
    if ( 1 == m_result_num ) {
        m_candidate = ibus_engine_plugin_get_retval (m_lua_plugin);
        result = "";
        if ( m_candidate->content ) {
            result = m_candidate->content;
            if (strstr (result.c_str (), "\n"))
                result = "(字符画)";
        }
        if ( m_candidate->suggest && m_candidate-> help ) {
            result += m_candidate->suggest;
            result += " ";
            result += "[";
            result += m_candidate->help;
            result += "]";
        }

        m_lookup_table.appendCandidate (Text (result));
    }else if (m_result_num > 1) {
        m_candidates = ibus_engine_plugin_get_retvals (m_lua_plugin);
        for ( int i = 0; i < m_result_num; ++i) {
            const lua_command_candidate_t * candidate = g_array_index (m_candidates, lua_command_candidate_t *, i);
            result = "";
            if ( candidate->content ) {
                result = candidate->content;
                if (strstr (result.c_str (), "\n"))
                    result = "(字符画)";
            }
            if ( candidate->suggest && candidate-> help ) {
                result += candidate->suggest;
                result += " ";
                result += "[";
                result += candidate->help;
                result += "]";
            }

            m_lookup_table.appendCandidate (Text (result));
        }
    }

    return true;
}

bool
ExtEditor::fillChineseNumber(gint64 num)
{
    clearLookupTable();

    if ( LABEL_LIST_NUMBERS == m_mode) {
        for ( int i = 1; i <= 10; ++i )
            m_lookup_table.setLabel ( i - 1, Text (i - 1 + 'a') );
    }

    std::string result = simplified_number(num);
    if ( !result.empty() ){
        Text text(result);
        m_lookup_table.appendCandidate(text);
    }

    result = traditional_number(num);
    if ( !result.empty() ){
        Text text(result);
        m_lookup_table.appendCandidate(text);
    }

    result = simplest_cn_number(num);
    if ( !result.empty() ){
        Text text(result);
        m_lookup_table.appendCandidate(text);
    }

    return TRUE;
}

void
ExtEditor::clearLookupTable (void)
{
    m_lookup_table.clear ();
    m_lookup_table.setPageSize (m_config.pageSize ());
    m_lookup_table.setOrientation (m_config.orientation ());
}

void
ExtEditor::updateLookupTable (void)
{
    if (m_lookup_table.size ()) {
        Editor::updateLookupTableFast (m_lookup_table, TRUE);
    }
    else {
        hideLookupTable ();
    }
}

void
ExtEditor::updatePreeditText (void)
{
    if ( G_UNLIKELY(m_preedit_text.empty ()) ) {
        hidePreeditText ();
        return;
    }

    StaticText preedit_text (m_preedit_text);
    Editor::updatePreeditText (preedit_text, m_cursor, TRUE);
}

void
ExtEditor::updateAuxiliaryText (void)
{
    if ( G_UNLIKELY(m_auxiliary_text.empty ()) ) {
        hideAuxiliaryText ();
        return;
    }

    StaticText aux_text (m_auxiliary_text);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}

};


