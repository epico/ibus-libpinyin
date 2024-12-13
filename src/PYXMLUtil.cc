/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2024 Peng Wu <alexepico@gmail.com>
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

#include "PYXMLUtil.h"
#include <glib.h>
#include <gio/gio.h>
#ifdef ENABLE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

namespace PY {

struct EngineXMLVersion{
    gboolean in_version_tag;
    /* There are two versions in the XML file, only keep the first version here. */
    gchar * first_version;
};

static void engine_parser_start_element (GMarkupParseContext  *context,
                                         const gchar          *element_name,
                                         const gchar         **attribute_names,
                                         const gchar         **attribute_values,
                                         gpointer              user_data,
                                         GError              **error);
static void engine_parser_end_element   (GMarkupParseContext  *context,
                                         const gchar          *element_name,
                                         gpointer              user_data,
                                         GError              **error);
static void engine_parser_characters    (GMarkupParseContext  *context,
                                         const gchar          *text,
                                         gsize                 text_len,
                                         gpointer              user_data,
                                         GError              **error);
static void engine_parser_passthrough   (GMarkupParseContext  *context,
                                         const gchar          *passthrough_text,
                                         gsize                 text_len,
                                         gpointer              user_data,
                                         GError              **error);
static void engine_parser_error         (GMarkupParseContext  *context,
                                         GError               *error,
                                         gpointer              user_data);


/*
 * Parser
 */
static const GMarkupParser engine_xml_parser = {
    engine_parser_start_element,
    engine_parser_end_element,
    engine_parser_characters,
    engine_parser_passthrough,
    engine_parser_error
};

struct EngineXMLFile{
    gboolean in_version_tag;
    gchar * first_version;
};

static void
engine_parser_start_element (GMarkupParseContext *context,
                             const gchar         *element_name,
                             const gchar        **attribute_names,
                             const gchar        **attribute_values,
                             gpointer             user_data,
                             GError             **error)
{
    EngineXMLFile * xmlfile = (EngineXMLFile *) user_data;
    if (0 == strcmp(element_name, "version"))
        xmlfile->in_version_tag = TRUE;
}

static void
engine_parser_end_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           gpointer             user_data,
                           GError             **error)
{
    EngineXMLFile * xmlfile = (EngineXMLFile *) user_data;
    if (0 == strcmp(element_name, "version"))
        xmlfile->in_version_tag = FALSE;
}

static void
engine_parser_characters (GMarkupParseContext *context,
                          const gchar         *text,
                          gsize                text_len,
                          gpointer             user_data,
                          GError             **error)
{
    EngineXMLFile * xmlfile = (EngineXMLFile *) user_data;
    if (xmlfile->in_version_tag && xmlfile->first_version == NULL)
        xmlfile->first_version = g_strdup(text);
}

static void
engine_parser_passthrough (GMarkupParseContext  *context,
                           const gchar          *passthrough_text,
                           gsize                 text_len,
                           gpointer              user_data,
                           GError              **error)
{
}

static void
engine_parser_error (GMarkupParseContext *context,
                     GError              *error,
                     gpointer             user_data)
{
    g_printerr ("ERROR: %s\n", error->message);
}

gchar *
load_file_content(const gchar * filename)
{
    gboolean success = FALSE;
    GError *error = NULL;

    GFile *file = NULL;
    char *data = NULL;
    gsize len = 0;

    file = g_file_new_for_path (filename);
    success = g_file_load_contents (file, NULL, &data, &len, NULL, &error);
    g_object_unref (file);

    return data;
}

gboolean
parse_engine_version(const char * filename, gchar ** version)
{
    GMarkupParseContext *context = NULL;
    gboolean success = FALSE;
    GError *error = NULL;

    GFile *file = NULL;
    char *data = NULL;
    gsize len = 0;

    EngineXMLFile xmlfile;
    memset(&xmlfile, 0, sizeof(EngineXMLFile));

    file = g_file_new_for_path (filename);
    success = g_file_load_contents (file, NULL, &data, &len, NULL, &error);
    g_object_unref (file);
    context = g_markup_parse_context_new (&engine_xml_parser, G_MARKUP_DEFAULT_FLAGS, &xmlfile, NULL);
    success = g_markup_parse_context_parse (context, data, len, NULL);
    g_markup_parse_context_free (context);
    g_free (data);

    *version = xmlfile.first_version;
    return TRUE;
}

void
show_message(const char* summary, const char* details)
{
#ifdef ENABLE_LIBNOTIFY
    NotifyNotification* notice = notify_notification_new (summary, details, NULL);
    notify_notification_show (notice, NULL);
    g_object_unref (notice);
#else
    if (details == NULL)
        g_message ("%s\n", summary);
    else
        g_message ("%s\n%s\n", summary, details);
#endif
}

};
