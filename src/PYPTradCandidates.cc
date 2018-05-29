/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2018 Peng Wu <alexepico@gmail.com>
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

#include "PYPTradCandidates.h"
#include "PYSimpTradConverter.h"

using namespace PY;

gboolean
TraditionalCandidates::processCandidates (std::vector<EnhancedCandidate> & candidates)
{
    m_candidates.clear ();

    for (guint i = 0; i < candidates.size (); i++) {
        EnhancedCandidate & candidate = candidates[i];

        m_candidates.push_back (candidate);

        candidate.m_candidate_type = CANDIDATE_TRADITIONAL_CHINESE;
        candidate.m_candidate_id = i;
        String trad;
        SimpTradConverter::simpToTrad (candidate.c_str (), trad);
        candidate.m_display_string = trad;
    }

    return TRUE;
}

SelectCandidateAction
TraditionalCandidates::selectCandidate (EnhancedCandidate & candidate)
{
    assert (CANDIDATE_TRADITIONAL_CHINESE == candidate.m_candidate_type);

    SelectCandidateAction action = SELECT_CANDIDATE_ALREADY_HANDLED;

    action = selectCandidateInPhoneticEditor (candidate);

    if (SELECT_CANDIDATE_MODIFY_IN_PLACE_AND_COMMIT == action) {
        String trad;
        SimpTradConverter::simpToTrad (candidate.c_str (), trad);
        candidate.m_display_string = trad;
    }

    return action;
}
