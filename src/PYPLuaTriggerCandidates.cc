/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2018 Peng Wu <alexepico@gmail.com>
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

#include "PYPLuaTriggerCandidates.h"
#include <assert.h>
#include "PYString.h"
#include "PYConfig.h"
#include "PYPPhoneticEditor.h"

using namespace PY;

static const int MAXIMUM_NUM = 10;

LuaTriggerCandidates::LuaTriggerCandidates (Editor *editor)
{
    m_editor = editor;
}

gboolean
LuaTriggerCandidates::setLuaPlugin (IBusEnginePlugin *plugin)
{
    m_lua_plugin = plugin;
    return TRUE;
}

gboolean
LuaTriggerCandidates::processCandidates (std::vector<EnhancedCandidate> & candidates)
{
    if (!m_lua_plugin)
        return FALSE;

    m_candidates.clear ();

    std::vector<EnhancedCandidate>::iterator pos;
    for (pos = candidates.begin (); pos != candidates.end (); ++pos) {
        if (CANDIDATE_NBEST_MATCH != pos->m_candidate_type)
            break;
    }

    const char * lua_function_name = NULL;
    const char * text = m_editor->m_text;
    gchar * string = NULL;

    EnhancedCandidate enhanced;
    enhanced.m_candidate_type = CANDIDATE_LUA_TRIGGER;

    if (ibus_engine_plugin_match_input
        (m_lua_plugin, text, &lua_function_name)) {
        int num = ibus_engine_plugin_call (m_lua_plugin, lua_function_name, text);

        num = std::min (num, MAXIMUM_NUM);
        for (int i = 0; i < num; ++i) {
            string = ibus_engine_plugin_get_nth_result (m_lua_plugin, i);
            enhanced.m_display_string = string;
            enhanced.m_candidate_id = i;
            g_free (string);

            candidates.insert (pos, enhanced);
            m_candidates.push_back (enhanced);
        }

        ibus_engine_plugin_clear_results (m_lua_plugin);

        return TRUE;
    } else {
        int num_in_page = std::min
            (m_editor->m_config.pageSize (), (guint)candidates.size ());
        for (int j = 0; j < num_in_page; ++j) {
            text = candidates[j].m_display_string.c_str ();
            if (ibus_engine_plugin_match_candidate
                (m_lua_plugin, text, &lua_function_name)) {
                int num = ibus_engine_plugin_call (m_lua_plugin, lua_function_name, text);

                num = std::min (num, MAXIMUM_NUM);
                for (int i = 0; i < num; ++i) {
                    string = ibus_engine_plugin_get_nth_result (m_lua_plugin, i);
                    enhanced.m_display_string = string;
                    enhanced.m_candidate_id = i;
                    g_free (string);

                    candidates.insert (pos, enhanced);
                    m_candidates.push_back (enhanced);
                }

                ibus_engine_plugin_clear_results (m_lua_plugin);

                return TRUE;
            }
        }
    }

    return FALSE;
}

int
LuaTriggerCandidates::selectCandidate (EnhancedCandidate & enhanced)
{
    assert (CANDIDATE_LUA_TRIGGER == enhanced.m_candidate_type);
    assert (enhanced.m_candidate_id < m_candidates.size ());

    return SELECT_CANDIDATE_COMMIT;
}

gboolean
LuaTriggerCandidates::removeCandidate (EnhancedCandidate & enhanced)
{
    assert (CANDIDATE_LUA_TRIGGER == enhanced.m_candidate_type);
    assert (enhanced.m_candidate_id < m_candidates.size ());

    return FALSE;
}
