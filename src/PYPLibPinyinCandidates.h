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

#ifndef __PY_LIB_PINYIN_LIB_PINYIN_CANDIDATES_H_
#define __PY_LIB_PINYIN_LIB_PINYIN_CANDIDATES_H_

#include "PYPEnhancedCandidates.h"

namespace PY {

class LibPinyinCandidates : public EnhancedCandidate {
public:
    LibPinyinCandidates (PhoneticEditor *editor) {
        m_editor = editor;
    }

public:
    gboolean processCandidates (std::vector<EnhancedCandidate> & candidates) {
        pinyin_instance_t *instance = m_editor->m_instance;

        guint len = 0;
        pinyin_get_n_candidate (m_instance, &len);

        for (guint i = 0; i < len; i++) {
            lookup_candidate_t * candidate = NULL;
            pinyin_get_candidate (m_instance, i, &candidate);

            const gchar * phrase_string = NULL;
            pinyin_get_candidate_string (m_instance, candidate, &phrase_string);

            EnhancedCandidate candidate;
            candidate.m_candidate_type = CANDIDATE_LIBPINYIN;
            candidate.m_candidate_id = i;
            candidate.m_display_string = phrase_string;

            candidates.push_back (candidate);
        }

        return TRUE;
    }

    SelectCandidateAction selectCandidate (EnhancedCandidate & candidate) {
        assert (CANDIDATE_LIBPINYIN == candidate.m_candidate_type);

        assert (FALSE);
    }

};

};

#endif
