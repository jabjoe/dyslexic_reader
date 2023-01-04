/*
Copyright (C) 2016  Joe Burmeister

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdlib.h>
#include <gtkspell/gtkspell.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "reader.h"
#include "resources.h"


static GtkWidget   * read_btn     = NULL;
static GtkWidget   * stop_btn     = NULL;
static GtkWidget   * settings_btn = NULL;
static GtkWidget   * main_window  = NULL;
static GtkTextView * text_view    = NULL;
static GtkBuilder  * builder      = NULL;
static GtkTextBuffer     * text_buffer = NULL;
static GtkTextTag        * speaking_tag = NULL;
static GtkScrolledWindow * scroll_area = NULL;

static GtkTextIter     speaking_start;
static GtkTextIter     speaking_end;

static GtkTextIter     speaking_range_start;
static GtkTextIter     speaking_range_end;

static int start_offset = 0;

static int pipe_ipc[2];

static bool in_debug = false;


typedef struct
{
    const char* voice;
    const char* language;
    const char* spell_lang;
    double rate;
} Settings;

Settings settings;



extern void read_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    if (dyslexic_reader_is_reading(reader))
    {
        dyslexic_reader_start_stop(reader);
    }
    else
    {
        if (gtk_text_buffer_get_has_selection(text_buffer))
        {
            if (!gtk_text_buffer_get_selection_bounds(text_buffer,
                                                &speaking_range_start,
                                                &speaking_range_end))
            {
                gtk_text_buffer_get_iter_at_offset (text_buffer,
                                            &speaking_range_start, 0);
                gtk_text_buffer_get_iter_at_offset (text_buffer,
                                            &speaking_range_end, -1);
            }
            GtkTextIter it_a, it_b;
            gtk_text_buffer_get_iter_at_offset (text_buffer, &it_a, -1);
            gtk_text_buffer_get_iter_at_offset (text_buffer, &it_b, -1);

            gtk_text_buffer_select_range(text_buffer, &it_a, &it_b);
        }
        else
        {
            gtk_text_buffer_get_iter_at_offset (text_buffer,
                                            &speaking_range_start, 0);
            gtk_text_buffer_get_iter_at_offset (text_buffer,
                                            &speaking_range_end, -1);
        }

        start_offset = gtk_text_iter_get_offset(&speaking_range_start);
        int end_offset = gtk_text_iter_get_offset(&speaking_range_end);

        const char* text_start = gtk_text_buffer_get_text(text_buffer,
                                                &speaking_range_start,
                                                &speaking_range_end,
                                                FALSE);
        const char* text_end = text_start;

        int count = end_offset - start_offset;

        while(count--)
            text_end = g_utf8_next_char(text_end);

        if (dyslexic_reader_start_read(reader, text_start, text_end))
        {
            gtk_text_buffer_get_iter_at_offset(text_buffer, &speaking_start, 0);
            gtk_text_buffer_get_iter_at_offset(text_buffer, &speaking_end, 0);
            gtk_text_view_set_editable (text_view, FALSE);
            gtk_widget_hide(read_btn);
            gtk_widget_show_all(stop_btn);
            gtk_widget_set_sensitive(settings_btn, FALSE);
        }
    }
}


extern void stop_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    dyslexic_reader_start_stop(reader);
}


extern void settings_ok_btn_clicked_cb(GtkButton* btn, GtkDialog * settings_dialog )
{
    gtk_dialog_response(settings_dialog, GTK_RESPONSE_OK);
}


extern void settings_cancel_btn_clicked_cb(GtkButton* btn, GtkDialog * settings_dialog )
{
    gtk_dialog_response(settings_dialog, GTK_RESPONSE_CANCEL);
}


static void update_settings()
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(main_window), "reader");
    GtkSpellChecker *spellcheck = (GtkSpellChecker*) g_object_get_data(G_OBJECT(main_window), "spellcheck");

    if (!gtk_spell_checker_set_language (spellcheck, settings.spell_lang, NULL))
        g_warning("Failed to change spelling to %s", settings.spell_lang);

    dyslexic_reader_set_rate(reader, settings.rate);
    if (!dyslexic_reader_set_voice(reader, settings.voice))
        g_warning("Failed to change voice to %s", settings.voice);

    if (!dyslexic_reader_set_language(reader, settings.language))
    {
        g_warning("Failed to change language to %s", settings.language);
        if (!dyslexic_reader_set_language(reader, "en"))
            g_warning("Failed to even set language to english.");
    }
}


static void load_settings()
{
    GSettings * gsettings = g_settings_new("org.dyslexicreader");
    if (gsettings)
    {
        settings.rate       = g_settings_get_double(gsettings, "rate");
        settings.voice      = g_settings_get_string(gsettings, "voice");
        settings.language   = g_settings_get_string(gsettings, "language");
        settings.spell_lang = g_settings_get_string(gsettings, "spelllang");
    }
    g_object_unref(G_OBJECT(gsettings));
    update_settings();
}


static void save_settings()
{
    GSettings * gsettings = g_settings_new("org.dyslexicreader");
    if (gsettings)
    {
        g_settings_set_double(gsettings, "rate", settings.rate);
        g_settings_set_string(gsettings, "voice", settings.voice);
        g_settings_set_string(gsettings, "language", settings.language);
        g_settings_set_string(gsettings, "spelllang", settings.spell_lang);
        g_settings_sync();
    }
    g_object_unref(G_OBJECT(gsettings));
}


extern void about_btn_clicked_cb(GtkButton* btn, GtkAboutDialog * about_dialog )
{
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_hide(GTK_WIDGET(about_dialog));
}


extern void settings_btn_clicked_cb(GtkButton* btn, GtkDialog * settings_dialog )
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(main_window), "reader");
    GtkSpellChecker *spellcheck = (GtkSpellChecker*) g_object_get_data(G_OBJECT(main_window), "spellcheck");

    GtkComboBoxText* voices_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "voice_dropdown"));
    GtkComboBoxText* language_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "language_dropdown"));
    GtkComboBoxText* spelling_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "spell_dropdown"));
    GtkAdjustment* speed_spin_adj = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "speed_spin_adj"));

    gtk_combo_box_text_remove_all(voices_ui);
    gtk_combo_box_text_remove_all(language_ui);
    gtk_combo_box_text_remove_all(spelling_ui);

    GList *spell_langs = gtk_spell_checker_get_language_list();

    const char* const * voices = dyslexic_reader_list_voices(reader);
    const char* const * p = voices;

    if (voices)
    {
        for(;*p; p++)
        {
            gtk_combo_box_text_append_text(voices_ui,  *p);
            if (settings.voice && !strcmp(*p, settings.voice))
                gtk_combo_box_set_active(GTK_COMBO_BOX(voices_ui), p - voices);
        }
    } else g_warning("Failed to find any voices.");

    const char* const * languages = dyslexic_reader_list_languages(reader);
    if (languages)
    {
        for(int n = 0; languages[n]; ++n)
        {
            gtk_combo_box_text_append_text(language_ui,  languages[n]);
            if (settings.language && !strcmp(languages[n], settings.language))
                gtk_combo_box_set_active(GTK_COMBO_BOX(language_ui), n);
        }
    }
    else g_warning("Failed to find any languages.");

    int i = 0;
    for(GList* c = spell_langs; c; c = c->next, ++i)
    {
        gtk_combo_box_text_append_text(spelling_ui, (const char*)c->data);
        if (!strcmp((const char*)c->data, gtk_spell_checker_get_language(spellcheck)))
            gtk_combo_box_set_active(GTK_COMBO_BOX(spelling_ui), i);
    }

    gtk_adjustment_set_value( speed_spin_adj, settings.rate);

    gint result = gtk_dialog_run(settings_dialog);

    gtk_widget_hide (GTK_WIDGET(settings_dialog));

    if (result == GTK_RESPONSE_OK)
    {
        int lang_index = gtk_combo_box_get_active(GTK_COMBO_BOX(language_ui));
        if (lang_index < 0)
            g_warning("Failed to find selected language.....");
        else
            settings.language = languages[lang_index];

        int voice_index = gtk_combo_box_get_active(GTK_COMBO_BOX(voices_ui));
        if (voice_index < 0)
            g_warning("Failed to find selected voice.....");
        else
            settings.voice = voices[voice_index];

        settings.rate = gtk_adjustment_get_value (speed_spin_adj);

        i = 0;
        for(GList* c = spell_langs; c; c = c->next, ++i)
            if (i == gtk_combo_box_get_active(GTK_COMBO_BOX(spelling_ui)))
            {
                if (settings.spell_lang)
                    free((void*)settings.spell_lang);
                settings.spell_lang = strdup((const char*)c->data);
            }

        update_settings();
        save_settings();
    }
}


static void safe_write(int fd, void* data, size_t len)
{
    int written = 0;
    while(written < len)
    {
        int r = write(fd, data + written, len - written);
        if (r == -1)
        {
            if (errno != EAGAIN && errno != EINTR)
                continue;
            fprintf(stderr, "Safe write failed : %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else written += r;
    }
}


static void safe_read(int fd, void* data, size_t len)
{
    int read_bytes = 0;
    while(read_bytes < len)
    {
        int r = read(fd, data + read_bytes, len - read_bytes);
        if (r == -1)
        {
            if (errno != EAGAIN && errno != EINTR)
                continue;
            fprintf(stderr, "Safe read failed : %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else read_bytes += r;
    }
}


void reading_stopped(dyslexic_reader_t *reader)
{
    uint start = -1;
    uint end = -1;

    safe_write(pipe_ipc[1], &start, sizeof(start));
    safe_write(pipe_ipc[1], &end, sizeof(end));
}


void reading_updated(dyslexic_reader_t* reader, uint start, uint end)
{
    safe_write(pipe_ipc[1], &start, sizeof(start));
    safe_write(pipe_ipc[1], &end, sizeof(end));
}


void reading_debug(char * fmt, ...)
{
    if (!in_debug)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}


gboolean ipc_pipe_update_cb(gint fd,
                          GIOCondition condition,
                          gpointer user_data)
{
    uint start, end;

    safe_read(fd, &start, sizeof(start));
    safe_read(fd, &end, sizeof(end));

    gtk_text_buffer_remove_tag(text_buffer,
                           speaking_tag,
                           &speaking_start,
                           &speaking_end);

    if (start == -1 && end == -1)
    {
        dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(main_window), "reader");
        dyslexic_reader_finsihed(reader);

        gtk_widget_show_all(read_btn);
        gtk_widget_hide(stop_btn);
        gtk_text_view_set_editable (text_view, TRUE);
        gtk_widget_set_sensitive(settings_btn, TRUE);

        if (!gtk_text_iter_is_start(&speaking_range_start) ||
            !gtk_text_iter_is_end(&speaking_range_end))
        {
            gtk_text_buffer_select_range(text_buffer,
                                        &speaking_range_start,
                                        &speaking_range_end);
        }
    }
    else
    {
        gtk_text_iter_set_offset(&speaking_start, start + start_offset);
        gtk_text_iter_set_offset(&speaking_end, end + start_offset);

        gtk_text_buffer_apply_tag(text_buffer,
                                  speaking_tag,
                                  &speaking_start,
                                  &speaking_end);

        GdkRectangle rect = {0};

        gtk_text_view_get_iter_location(text_view, &speaking_end, &rect);

        GtkAdjustment * adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE(text_view));

        if (adj)
        {
            gdouble half_page = gtk_adjustment_get_page_size (adj) / 2;
            gdouble value     = gtk_adjustment_get_value (adj);

            if ((rect.y + rect.height) > (value + half_page))
                gtk_adjustment_set_value(adj, MIN(value+half_page/2, gtk_adjustment_get_upper(adj)));
            else if (rect.y < value)
                gtk_adjustment_set_value(adj, rect.y);
        }

        gtk_widget_show_all(GTK_WIDGET(text_view));
    }
    return TRUE;
}


int main(int argc, char* argv[])
{
    gtk_init (&argc, &argv);

    in_debug = (getenv("DEBUG") != NULL)?true:false;

    memset(&settings, 0, sizeof(settings));

    GResource * res = resources_get_resource();

    g_resources_register(res);

    builder = gtk_builder_new_from_resource ("/ui.glade");

    if (!builder)
    {
        g_critical("GtkBuilder object allocation failed.");
        return EXIT_FAILURE;
    }

    main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    if (!main_window)
    {
        g_critical("Dyslexic reader requires \"main_window\" from GtkBuilder.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (builder, "text_buffer"));
    if (!text_buffer)
    {
        g_critical("Dyslexic reader requires \"text_buffer\" from GtkBuilder.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    speaking_tag = gtk_text_buffer_create_tag(text_buffer, "speaking", "background", "blue", "foreground", "yellow", NULL);
    if (!speaking_tag)
    {
        g_critical("Dyslexic reader failed to create speaking tag.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    read_btn     = GTK_WIDGET (gtk_builder_get_object (builder, "read_btn"));
    stop_btn     = GTK_WIDGET (gtk_builder_get_object (builder, "stop_btn"));
    settings_btn = GTK_WIDGET (gtk_builder_get_object (builder, "settings_btn"));
    text_view = GTK_TEXT_VIEW (gtk_builder_get_object (builder, "text_view"));
    scroll_area = GTK_SCROLLED_WINDOW (gtk_builder_get_object (builder, "scrolledwindow1"));

    if (!read_btn || !text_view || !settings_btn || !scroll_area)
    {
        g_critical("Dyslexic reader has not found are required widgets from GtkBuilder.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    GtkSpellChecker *spellcheck = gtk_spell_checker_new();
    if (!spellcheck)
    {
        g_critical("Dyslexic reader failed to create spell check.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    gtk_spell_checker_attach(spellcheck, text_view);

    gtk_builder_connect_signals (builder, NULL);

    if (pipe(pipe_ipc) != 0)
    {
        g_critical("Dyslexic reader failed to set up IPC pipe");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    dyslexic_reader_t* reader = dyslexic_reader_create();

    if (reader)
    {
        g_unix_fd_add(pipe_ipc[0], G_IO_IN, ipc_pipe_update_cb, reader);

        g_object_set_data(G_OBJECT(main_window), "reader", reader);
        g_object_set_data(G_OBJECT(main_window), "spellcheck", spellcheck);

        load_settings();

        gtk_widget_show (main_window);
        gtk_main ();
        dyslexic_reader_destroy(reader);
    }

    g_object_unref(builder);
    g_resources_unregister(res);
    g_resource_unref(res);

    if (settings.spell_lang)
        free((void*)settings.spell_lang);

    return EXIT_SUCCESS;
}
