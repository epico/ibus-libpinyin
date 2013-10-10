/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
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
#include "PYPPinyinEngine.h"
#include <string>
#include "PYConfig.h"
#include "PYPConfig.h"
#include "PYPunctEditor.h"
#include "PYRawEditor.h"
#ifdef IBUS_BUILD_LUA_EXTENSION
#include "PYExtEditor.h"
#endif
#ifdef IBUS_BUILD_ENGLISH_INPUT_MODE
#include "PYEnglishEditor.h"
#endif
#ifdef IBUS_BUILD_STROKE_INPUT_MODE
#include "PYStrokeEditor.h"
#endif
#include "PYPFullPinyinEditor.h"
#include "PYPDoublePinyinEditor.h"
#include "PYFallbackEditor.h"

using namespace PY;

/* constructor */
LibPinyinPinyinEngine::LibPinyinPinyinEngine (IBusEngine *engine)
    : Engine (engine),
      m_props (LibPinyinPinyinConfig::instance ()),
      m_prev_pressed_key (IBUS_VoidSymbol),
      m_input_mode (MODE_INIT),
      m_fallback_editor (new FallbackEditor (m_props, LibPinyinPinyinConfig::instance ()))
{
    gint i;

#if IBUS_CHECK_VERSION (1, 5, 4)
    m_input_purpose = IBUS_INPUT_PURPOSE_FREE_FORM;
#endif

    m_double_pinyin = LibPinyinPinyinConfig::instance ().doublePinyin ();

    if (m_double_pinyin)
        m_editors[MODE_INIT].reset
            (new LibPinyinDoublePinyinEditor (m_props, LibPinyinPinyinConfig::instance ()));
    else
        m_editors[MODE_INIT].reset
            (new LibPinyinFullPinyinEditor (m_props, LibPinyinPinyinConfig::instance ()));

    m_editors[MODE_PUNCT].reset
        (new PunctEditor (m_props, LibPinyinPinyinConfig::instance ()));
    m_editors[MODE_RAW].reset
        (new RawEditor (m_props, LibPinyinPinyinConfig::instance ()));

#ifdef IBUS_BUILD_LUA_EXTENSION
    m_editors[MODE_EXTENSION].reset (new ExtEditor (m_props, LibPinyinPinyinConfig::instance ()));
#else
    m_editors[MODE_EXTENSION].reset (new Editor (m_props, LibPinyinPinyinConfig::instance ()));
#endif
#ifdef IBUS_BUILD_ENGLISH_INPUT_MODE
    m_editors[MODE_ENGLISH].reset (new EnglishEditor (m_props, LibPinyinPinyinConfig::instance ()));
#else
    m_editors[MODE_ENGLISH].reset (new Editor (m_props, LibPinyinPinyinConfig::instance ()));
#endif
#ifdef IBUS_BUILD_STROKE_INPUT_MODE
    m_editors[MODE_STROKE].reset (new StrokeEditor (m_props, LibPinyinPinyinConfig::instance ()));
#else
    m_editors[MODE_STROKE].reset (new Editor (m_props, LibPinyinPinyinConfig::instance ()));
#endif

    m_props.signalUpdateProperty ().connect
        (std::bind (&LibPinyinPinyinEngine::updateProperty, this, _1));

    for (i = MODE_INIT; i < MODE_LAST; i++) {
        connectEditorSignals (m_editors[i]);
    }

    connectEditorSignals (m_fallback_editor);
}

/* destructor */
LibPinyinPinyinEngine::~LibPinyinPinyinEngine (void)
{
}

gboolean
LibPinyinPinyinEngine::processKeyEvent (guint keyval, guint keycode, guint modifiers)
{
    gboolean retval = FALSE;

#if IBUS_CHECK_VERSION (1, 5, 4)
    if (IBUS_INPUT_PURPOSE_PASSWORD == m_input_purpose)
        return retval;
#endif

    /* check Shift or Ctrl + Release hotkey,
     * and then ignore other Release key event */
    if (modifiers & IBUS_RELEASE_MASK) {
        /* press and release keyval are same,
         * and no other key event between the press and release key event */
        gboolean triggered = FALSE;

        if (m_prev_pressed_key == keyval){
            if (LibPinyinPinyinConfig::instance ().ctrlSwitch ()) {
                if (keyval == IBUS_Control_L || keyval == IBUS_Control_R)
                    triggered = TRUE;
            } else {
                if (keyval == IBUS_Shift_L || keyval == IBUS_Shift_R)
                    triggered = TRUE;
            }
        }

        if (triggered) {
            if (!m_editors[MODE_INIT]->text ().empty ())
                m_editors[MODE_INIT]->reset ();
            m_props.toggleModeChinese ();
            return TRUE;
        }

        if (m_input_mode == MODE_INIT &&
            m_editors[MODE_INIT]->text ().empty ()) {
            /* If it is in init mode, and  no any previous input text,
             * we will let client applications to handle release key event */
            return FALSE;
        }
        else {
            return TRUE;
        }
    }

    /* Toggle simp/trad Chinese Mode when hotkey Ctrl + Shift + F pressed */
    if (keyval == IBUS_F && scmshm_test (modifiers, (IBUS_SHIFT_MASK | IBUS_CONTROL_MASK))) {
        m_props.toggleModeSimp ();
        m_prev_pressed_key = IBUS_F;
        return TRUE;
    }

    if (m_props.modeChinese ()) {
        if (m_input_mode == MODE_INIT &&
            (cmshm_filter (modifiers) == 0)) {
            const String & text = m_editors[MODE_INIT]->text ();
            if (text.empty ()) {
                switch (keyval) {
                case IBUS_grave:
                    m_input_mode = MODE_PUNCT;
                    break;
#ifdef IBUS_BUILD_LUA_EXTENSION
                case IBUS_i:
                    // do not enable lua extension when use double pinyin.
                    if (LibPinyinPinyinConfig::instance ().doublePinyin ())
                        break;
                    m_input_mode = MODE_EXTENSION;
                    break;
#endif
#ifdef IBUS_BUILD_ENGLISH_INPUT_MODE
                case IBUS_v:
                    // do not enable english mode when use double pinyin.
                    if (LibPinyinPinyinConfig::instance ().doublePinyin ())
                        break;
                    m_input_mode = MODE_ENGLISH;
                    break;
#endif
#ifdef IBUS_BUILD_STROKE_INPUT_MODE
                case IBUS_u:
                    // do not enable stroke mode when use double pinyin.
                    if (LibPinyinPinyinConfig::instance ().doublePinyin ())
                        break;
                    m_input_mode = MODE_STROKE;
                    break;
#endif
                }
            } else {
                /* TODO: Unknown */
            }
        }
        retval = m_editors[m_input_mode]->processKeyEvent (keyval, keycode, modifiers);
        if (G_UNLIKELY (retval &&
                        m_input_mode != MODE_INIT &&
                        m_editors[m_input_mode]->text ().empty ()))
            m_input_mode = MODE_INIT;
    }

    if (G_UNLIKELY (!retval))
        retval = m_fallback_editor->processKeyEvent (keyval, keycode, modifiers);

    /* store ignored key event by editors */
    m_prev_pressed_key = retval ? IBUS_VoidSymbol : keyval;

    return retval;
}

void
LibPinyinPinyinEngine::focusIn (void)
{
    /* TODO: check memory leak here,
     *       or switch full/double pinyin when pinyin config is changed.*/
    if (LibPinyinPinyinConfig::instance ().doublePinyin ()) {
        if (!m_double_pinyin) {
            m_editors[MODE_INIT].reset (new LibPinyinDoublePinyinEditor (m_props, LibPinyinPinyinConfig::instance ()));
            connectEditorSignals (m_editors[MODE_INIT]);
        }
        m_double_pinyin = TRUE;
    }
    else {
        if (m_double_pinyin) {
            m_editors[MODE_INIT].reset (new LibPinyinFullPinyinEditor (m_props, LibPinyinPinyinConfig::instance ()));
            connectEditorSignals (m_editors[MODE_INIT]);
        }
        m_double_pinyin = FALSE;
    }

    registerProperties (m_props.properties ());
}

void
LibPinyinPinyinEngine::focusOut (void)
{

#if IBUS_CHECK_VERSION (1, 5, 4)
    m_input_purpose = IBUS_INPUT_PURPOSE_FREE_FORM;
#endif

    reset ();
}

#if IBUS_CHECK_VERSION(1, 5, 4)
void
LibPinyinPinyinEngine::setContentType (guint purpose, guint hints)
{
    m_input_purpose = (IBusInputPurpose) purpose;
}
#endif

void
LibPinyinPinyinEngine::reset (void)
{
    m_prev_pressed_key = IBUS_VoidSymbol;
    m_input_mode = MODE_INIT;
    for (gint i = 0; i < MODE_LAST; i++) {
        m_editors[i]->reset ();
    }
    m_fallback_editor->reset ();
}

void
LibPinyinPinyinEngine::enable (void)
{
    m_props.reset ();
}

void
LibPinyinPinyinEngine::disable (void)
{
}

void
LibPinyinPinyinEngine::pageUp (void)
{
    m_editors[m_input_mode]->pageUp ();
}

void
LibPinyinPinyinEngine::pageDown (void)
{
    m_editors[m_input_mode]->pageDown ();
}

void
LibPinyinPinyinEngine::cursorUp (void)
{
    m_editors[m_input_mode]->cursorUp ();
}

void
LibPinyinPinyinEngine::cursorDown (void)
{
    m_editors[m_input_mode]->cursorDown ();
}

inline void
LibPinyinPinyinEngine::showSetupDialog (void)
{
    g_spawn_command_line_async
        (LIBEXECDIR"/ibus-setup-libpinyin pinyin", NULL);
}

gboolean
LibPinyinPinyinEngine::propertyActivate (const char *prop_name, guint prop_state)
{
    const static String setup ("setup");
    if (m_props.propertyActivate (prop_name, prop_state)) {
        return TRUE;
    }
    else if (setup == prop_name) {
        showSetupDialog ();
        return TRUE;
    }
    return FALSE;
}

void
LibPinyinPinyinEngine::candidateClicked (guint index, guint button, guint state)
{
    m_editors[m_input_mode]->candidateClicked (index, button, state);
}

void
LibPinyinPinyinEngine::commitText (Text & text)
{
    Engine::commitText (text);
    if (m_input_mode != MODE_INIT)
        m_input_mode = MODE_INIT;
#if 1
    /* handle "<num>+.<num>+" here */
    if (text.text ())
        static_cast<FallbackEditor*> (m_fallback_editor.get ())->setPrevCommittedChar (*text.text ());
    else
        static_cast<FallbackEditor*> (m_fallback_editor.get ())->setPrevCommittedChar (0);
#endif
}

void
LibPinyinPinyinEngine::connectEditorSignals (EditorPtr editor)
{
    editor->signalCommitText ().connect (
        std::bind (&LibPinyinPinyinEngine::commitText, this, _1));

    editor->signalUpdatePreeditText ().connect (
        std::bind (&LibPinyinPinyinEngine::updatePreeditText, this, _1, _2, _3));
    editor->signalShowPreeditText ().connect (
        std::bind (&LibPinyinPinyinEngine::showPreeditText, this));
    editor->signalHidePreeditText ().connect (
        std::bind (&LibPinyinPinyinEngine::hidePreeditText, this));

    editor->signalUpdateAuxiliaryText ().connect (
        std::bind (&LibPinyinPinyinEngine::updateAuxiliaryText, this, _1, _2));
    editor->signalShowAuxiliaryText ().connect (
        std::bind (&LibPinyinPinyinEngine::showAuxiliaryText, this));
    editor->signalHideAuxiliaryText ().connect (
        std::bind (&LibPinyinPinyinEngine::hideAuxiliaryText, this));

    editor->signalUpdateLookupTable ().connect (
        std::bind (&LibPinyinPinyinEngine::updateLookupTable, this, _1, _2));
    editor->signalUpdateLookupTableFast ().connect (
        std::bind (&LibPinyinPinyinEngine::updateLookupTableFast, this, _1, _2));
    editor->signalShowLookupTable ().connect (
        std::bind (&LibPinyinPinyinEngine::showLookupTable, this));
    editor->signalHideLookupTable ().connect (
        std::bind (&LibPinyinPinyinEngine::hideLookupTable, this));
}
