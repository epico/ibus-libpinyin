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
#ifndef __PY_PROPERTY_H_
#define __PY_PROPERTY_H_

#include <ibus.h>
#include "PYObject.h"
#include "PYText.h"

namespace PY {

class Property : public Object {
public:
    Property (const gchar   *key,
              IBusPropType   type = PROP_TYPE_NORMAL,
              IBusText      *label = NULL,
              const gchar   *icon = NULL,
              IBusText      *tooltip = NULL,
              gboolean       sensitive = TRUE,
              gboolean       visible = TRUE,
              IBusPropState  state = PROP_STATE_UNCHECKED,
              IBusPropList  *props = NULL)
        : Object (ibus_property_new (key, type, label, icon, tooltip, sensitive, visible, state, props)) { }

    const gchar * getKey (void)
    {
        return ibus_property_get_key (get<IBusProperty> ());
    }

    void setLabel (IBusText *text)
    {
        ibus_property_set_label (get<IBusProperty> (), text);
    }

    void setLabel (const gchar *text)
    {
        setLabel (Text (text));
    }

    void setIcon (const gchar *icon)
    {
        ibus_property_set_icon (get<IBusProperty> (), icon);
    }

    void setSymbol (IBusText *text)
    {
        ibus_property_set_symbol (get<IBusProperty> (), text);
    }

    void setSymbol (const gchar *text)
    {
        setSymbol (Text (text));
    }

    void setSensitive (gboolean sensitive)
    {
        ibus_property_set_sensitive (get<IBusProperty> (), sensitive);
    }

    void setTooltip (IBusText *text)
    {
        ibus_property_set_tooltip (get<IBusProperty> (), text);
    }

    void setTooltip (const gchar *text)
    {
        setTooltip (Text (text));
    }

    void setState (IBusPropState state)
    {
        ibus_property_set_state (get<IBusProperty> (), state);
    }

    void setSubProps (IBusPropList *props)
    {
        ibus_property_set_sub_props (get<IBusProperty> (), props);
    }

    operator IBusProperty * (void) const
    {
        return get<IBusProperty> ();
    }
};


class PropList : Object {
public:
    PropList (void) : Object (ibus_prop_list_new ()) { }

    void append (Property &prop)
    {
        ibus_prop_list_append (get<IBusPropList> (), prop);
    }

    operator IBusPropList * (void) const
    {
        return get<IBusPropList> ();
    }
};

};

#endif
