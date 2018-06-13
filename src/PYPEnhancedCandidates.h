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

#ifndef __PY_LIB_PINYIN_ENHANCED_CANDIDATES_H_
#define __PY_LIB_PINYIN_ENHANCED_CANDIDATES_H_

#include <glib.h>
#include <string>
#include <vector>

namespace PY {

class PhoneticEditor;

enum CandidateType {
    CANDIDATE_NBEST_MATCH = 1,
    /* not included with user candidate */
    CANDIDATE_NORMAL,
    /* both normal candidate and user candidate */
    CANDIDATE_USER,
    CANDIDATE_TRADITIONAL_CHINESE,
    CANDIDATE_LUA_TRIGGER,
    CANDIDATE_LUA_CONVERTER,
    CANDIDATE_CLOUD_INPUT,
    CANDIDATE_EMOJI
};

enum SelectCandidateAction {
    SELECT_CANDIDATE_ALREADY_HANDLED = 1,
    /* commit the text without change. */
    SELECT_CANDIDATE_COMMIT,
    /* modify the candidate recursively for candidates process chain,
       then commit the changed text. */
    SELECT_CANDIDATE_MODIFY_IN_PLACE_AND_COMMIT,
    /* need to call updateCandidates method in class PhoneticEditor. */
    SELECT_CANDIDATE_UPDATE_ALL
};

struct EnhancedCandidate {
    CandidateType m_candidate_type;
    guint m_candidate_id;
    std::string m_display_string;
};

class EnhancedCandidates {

public:
    gboolean processCandidates (std::vector<EnhancedCandidate> & candidates);

    SelectCandidateAction selectCandidate (EnhancedCandidate & enhanced);

protected:

    /* will call selectCandidateInternal method of class PhoneticEditor. */
    PhoneticEditor *m_editor;
};

};

#endif
