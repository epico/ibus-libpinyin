/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "PYPPinyinEditor.h"
#include "PYConfig.h"
#include "PYPinyinProperties.h"
#include "PYSimpTradConverter.h"
#include "PYHalfFullConverter.h"
#include "PYLibPinyin.h"

using namespace PY;

/* init static members*/
LibPinyinPinyinEditor::LibPinyinPinyinEditor (PinyinProperties & props,
                                              Config & config)
    : LibPinyinPhoneticEditor (props, config)
{
}


/**
 * process pinyin
 */
inline gboolean
LibPinyinPinyinEditor::processPinyin (guint keyval, guint keycode,
                                      guint modifiers)
{
    if (G_UNLIKELY (cmshm_filter (modifiers) != 0))
        return m_text ? TRUE : FALSE;

    return insert (keyval);
}

/**
 * process numbers
 */
inline gboolean
LibPinyinPinyinEditor::processNumber (guint keyval, guint keycode,
                                      guint modifiers)
{
    guint i;

    if (m_text.empty ())
        return FALSE;

    switch (keyval) {
    case IBUS_0:
    case IBUS_KP_0:
        i = 9;
        break;
    case IBUS_1 ... IBUS_9:
        i = keyval - IBUS_1;
        break;
    case IBUS_KP_1 ... IBUS_KP_9:
        i = keyval - IBUS_KP_1;
        break;
    default:
        g_return_val_if_reached (FALSE);
    }

    if (modifiers == 0)
        selectCandidateInPage (i);

    update ();
    return TRUE;
}

inline gboolean
LibPinyinPinyinEditor::processPunct (guint keyval, guint keycode,
                                     guint modifiers)
{
    if (m_text.empty ())
        return FALSE;

    if (cmshm_filter (modifiers) != 0)
        return TRUE;

    switch (keyval) {
    case IBUS_apostrophe:
        return insert (keyval);
    case IBUS_comma:
        if (m_config.commaPeriodPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
    case IBUS_minus:
        if (m_config.minusEqualPage ()) {
            pageUp ();
            return TRUE;
        }
        break;
    case IBUS_period:
        if (m_config.commaPeriodPage ()) {
            pageDown ();
            return TRUE;
        }
        break;
    case IBUS_equal:
        if (m_config.minusEqualPage ()) {
            pageDown ();
            return TRUE;
        }
        break;
    }

#if 0
    if (m_config.autoCommit ()) {
        if (m_lookup_table.size ()) {
            /* TODO: check here. */
            selectCandidate (m_lookup_table.cursorPos ());
        }
        commit ();
        return FALSE;
    }
#endif

    return TRUE;
}

inline gboolean
LibPinyinPinyinEditor::processFunctionKey (guint keyval, guint keycode,
                                           guint modifiers)
{
    if (m_text.empty ())
        return FALSE;

    /* ignore numlock */
    modifiers = cmshm_filter (modifiers);

    if (modifiers != 0 && modifiers != IBUS_CONTROL_MASK)
        return TRUE;

    /* process some cursor control keys */
    if (modifiers == 0) { /* no modifiers. */
        switch (keyval) {
        case IBUS_Shift_L:
            if (!m_config.shiftSelectCandidate ())
                return FALSE;
            selectCandidateInPage (1);
            return TRUE;

        case IBUS_Shift_R:
            if (!m_config.shiftSelectCandidate ())
                return FALSE;
            selectCandidateInPage (2);
            return TRUE;
        }
    }

    return LibPinyinPhoneticEditor::processFunctionKey (keyval, keycode,
                                                        modifiers);
}

gboolean
LibPinyinPinyinEditor::processKeyEvent (guint keyval, guint keycode,
                                        guint modifiers)
{
    modifiers &= (IBUS_SHIFT_MASK |
                  IBUS_CONTROL_MASK |
                  IBUS_MOD1_MASK |
                  IBUS_SUPER_MASK |
                  IBUS_HYPER_MASK |
                  IBUS_META_MASK |
                  IBUS_LOCK_MASK);

    switch (keyval) {
    /* letters */
    case IBUS_a ... IBUS_z:
        return processPinyin (keyval, keycode, modifiers);
    case IBUS_0 ... IBUS_9:
    case IBUS_KP_0 ... IBUS_KP_9:
        return processNumber (keyval, keycode, modifiers);
    case IBUS_exclam ... IBUS_slash:
    case IBUS_colon ... IBUS_at:
    case IBUS_bracketleft ... IBUS_quoteleft:
    case IBUS_braceleft ... IBUS_asciitilde:
        return processPunct (keyval, keycode, modifiers);
    case IBUS_space:
        return processSpace (keyval, keycode, modifiers);
    default:
        return processFunctionKey (keyval, keycode, modifiers);
    }
}

void
LibPinyinPinyinEditor::commit ()
{
    if (G_UNLIKELY (m_text.empty ()))
        return;

    m_buffer.clear ();

    /* sentence candidate */
    char *tmp = NULL;
    pinyin_get_sentence (m_instance, &tmp);
    if (tmp) {
        if (m_props.modeSimp ()) {
            m_buffer << tmp;
        } else {
            SimpTradConverter::simpToTrad (tmp, m_buffer);
        }
        g_free (tmp);
    }

    /* text after pinyin */
    const gchar *p = m_text.c_str() + m_pinyin_len;
    if (G_UNLIKELY (m_props.modeFull ())) {
        while (*p != '\0') {
            m_buffer.appendUnichar (HalfFullConverter::toFull (*p++));
        }
    } else {
        m_buffer << p;
    }

    pinyin_train (m_instance);
    LibPinyinBackEnd::instance ().modified ();
    LibPinyinPhoneticEditor::commit ((const gchar *)m_buffer);
    reset();
}

void
LibPinyinPinyinEditor::updatePreeditText ()
{
    /* preedit text = guessed sentence + un-parsed pinyin text */
    if (G_UNLIKELY (m_text.empty ())) {
        hidePreeditText ();
        return;
    }

    m_buffer.clear ();
    char *tmp = NULL;
    pinyin_get_sentence (m_instance, &tmp);
    if (tmp) {
        if (m_props.modeSimp ()) {
            m_buffer<<tmp;
        } else {
            SimpTradConverter::simpToTrad (tmp, m_buffer);
        }
        g_free (tmp);
        tmp = NULL;
    }

    /* append rest text */
    const gchar *p = m_text.c_str () + m_pinyin_len;
    m_buffer << p;

    StaticText preedit_text (m_buffer);
    /* underline */
    preedit_text.appendAttribute (IBUS_ATTR_TYPE_UNDERLINE, IBUS_ATTR_UNDERLINE_SINGLE, 0, -1);

    guint pinyin_cursor = getPinyinCursor ();
    Editor::updatePreeditText (preedit_text, pinyin_cursor, TRUE);
}

void
LibPinyinPinyinEditor::updateAuxiliaryText ()
{
    if (G_UNLIKELY (m_text.empty ())) {
        hideAuxiliaryText ();
        return;
    }

    m_buffer.clear ();

    /* Note: cursor handling is defered to full/double pinyin editors. */

    guint len = 0;
    pinyin_get_n_pinyin (m_instance, &len);

    for (guint i = 0; i < len; ++i) {
        if (G_LIKELY (i))
            m_buffer << ' ';

        PinyinKey *key = NULL;
        pinyin_get_pinyin_key (m_instance, i, &key);

        gchar * str = NULL;
        pinyin_get_pinyin_string (m_instance, key, &str);

        m_buffer << str;
        g_free (str);
    }

    /* append rest text */
    const gchar *p = m_text.c_str() + m_pinyin_len;
    m_buffer << p;

    StaticText aux_text (m_buffer);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}

void
LibPinyinPinyinEditor::updateLookupTable ()
{
    m_lookup_table.setPageSize (m_config.pageSize ());
    m_lookup_table.setOrientation (m_config.orientation ());
    LibPinyinPhoneticEditor::updateLookupTable ();
}

