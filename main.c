#include <stdlib.h>
#include <gtkspell/gtkspell.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <string.h>
#include <errno.h>

#include "reader.h"
#include "resources.h"


GtkWidget   * read_btn     = NULL;
GtkWidget   * pause_btn    = NULL;
GtkWidget   * stop_btn     = NULL;
GtkWidget   * settings_btn = NULL;
GtkWidget   * main_window  = NULL;
GtkTextView * text_view    = NULL;
GtkBuilder  * builder      = NULL;
GtkTextBuffer     * text_buffer = NULL;
GtkScrolledWindow * scroll_area = NULL;

GtkTextIter     speaking_start;
GtkTextIter     speaking_end;

static int start_offset = 0;

static int pipe_ipc[2];


typedef struct
{
    const char* voice;
    const char* language;
    double rate;
} Settings;

Settings settings;



extern void read_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    if (dyslexic_reader_is_paused(reader))
    {
        if (dyslexic_reader_continue(reader))
        {
            gtk_widget_hide(read_btn);
            gtk_widget_show_all(pause_btn);
        }
        else dyslexic_reader_start_stop(reader);
    }
    else
    {
        GtkTextIter start, end;

        if (gtk_text_buffer_get_has_selection(text_buffer))
        {
            if (!gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end))
            {
                gtk_text_buffer_get_iter_at_offset (text_buffer, &start, 0);
                gtk_text_buffer_get_iter_at_offset (text_buffer, &end, -1);
            }
        }
        else
        {
            gtk_text_buffer_get_iter_at_offset (text_buffer, &start, 0);
            gtk_text_buffer_get_iter_at_offset (text_buffer, &end, -1);
        }

        start_offset = gtk_text_iter_get_offset(&start);
        int end_offset = gtk_text_iter_get_offset(&end);

        const char* text_start = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
        const char* text_end = text_start;

        while(end_offset--)
            text_end = g_utf8_next_char(text_end);

        if (dyslexic_reader_start_read(reader, text_start, text_end))
        {
            gtk_text_buffer_get_iter_at_offset(text_buffer, &speaking_start, 0);
            gtk_text_buffer_get_iter_at_offset(text_buffer, &speaking_end, 0);
            gtk_text_view_set_editable (text_view, FALSE);
            gtk_widget_hide(read_btn);
            gtk_widget_show_all(pause_btn);
            gtk_widget_set_sensitive(settings_btn, FALSE);
            gtk_widget_set_sensitive(stop_btn, TRUE);
        }
    }
}


extern void pause_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    if (dyslexic_reader_start_pause(reader))
    {
        gtk_widget_hide(pause_btn);
        gtk_widget_show_all(read_btn);
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

    printf("Setting to \"%s\" in \"%s\" at %f\n", settings.voice, settings.language, settings.rate);

    const char* const * long_langs = dyslexic_reader_list_languages(reader);
    const char* const * short_langs = dyslexic_reader_list_languages_short(reader);

    for(int n=0; long_langs[n]; ++n)
        if (!strcmp(long_langs[n], settings.language))
            gtk_spell_checker_set_language (spellcheck, short_langs[n], NULL);

    dyslexic_reader_set_rate(reader, settings.rate);
    dyslexic_reader_set_voice(reader, settings.voice);
    dyslexic_reader_set_language(reader, settings.language);
}


static void load_settings()
{
    GSettings * gsettings = g_settings_new("org.dyslexicreader");
    if (gsettings)
    {
        settings.rate       = g_settings_get_double(gsettings, "rate");
        settings.voice      = g_settings_get_string(gsettings, "voice");
        settings.language   = g_settings_get_string(gsettings, "language");
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
        g_settings_sync();
    }
    g_object_unref(G_OBJECT(gsettings));
}




extern void settings_btn_clicked_cb(GtkButton* btn, GtkDialog * settings_dialog )
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(main_window), "reader");

    GtkComboBoxText* voices_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "voice_dropdown"));
    GtkComboBoxText* language_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "language_dropdown"));
    GtkAdjustment* speed_spin_adj = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "speed_spin_adj"));

    gtk_combo_box_text_remove_all(voices_ui);
    gtk_combo_box_text_remove_all(language_ui);

    const char* const * voices = dyslexic_reader_list_voices(reader);
    const char* const * p = voices;

    for(;*p; p++)
    {
        gtk_combo_box_text_append_text(voices_ui,  *p);
        if (settings.voice && !strcmp(*p, settings.voice))
            gtk_combo_box_set_active(GTK_COMBO_BOX(voices_ui), p - voices);
    }

    const char* const * languages = dyslexic_reader_list_languages(reader);

    for(int n = 0; languages[n]; ++n)
    {
        gtk_combo_box_text_append_text(language_ui,  languages[n]);
        if (settings.language && !strcmp(languages[n], settings.language))
            gtk_combo_box_set_active(GTK_COMBO_BOX(language_ui), n);
    }

    gtk_adjustment_set_value( speed_spin_adj, settings.rate);

    gint result = gtk_dialog_run(settings_dialog);

    gtk_widget_hide (GTK_WIDGET(settings_dialog));

    if (result == GTK_RESPONSE_OK)
    {
        uint lang_index = gtk_combo_box_get_active(GTK_COMBO_BOX(language_ui));

        settings.voice = voices[gtk_combo_box_get_active(GTK_COMBO_BOX(voices_ui))];
        settings.language = languages[lang_index];
        settings.rate = gtk_adjustment_get_value (speed_spin_adj);

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


gboolean ipc_pipe_update_cb(gint fd,
                          GIOCondition condition,
                          gpointer user_data)
{
    uint start, end;

    safe_read(fd, &start, sizeof(start));
    safe_read(fd, &end, sizeof(end));

    if (start == -1 && end == -1)
    {
        gtk_text_iter_set_offset(&speaking_start, -1);
        gtk_text_iter_set_offset(&speaking_end, -1);

        gtk_text_buffer_select_range(gtk_text_view_get_buffer(text_view), &speaking_start, &speaking_end);
        gtk_widget_hide(pause_btn);
        gtk_widget_show_all(read_btn);
        gtk_text_view_set_editable (text_view, TRUE);
        gtk_widget_set_sensitive(settings_btn, TRUE);
        gtk_widget_set_sensitive(stop_btn, FALSE);
    }
    else
    {
        gtk_text_iter_set_offset(&speaking_start, start + start_offset);
        gtk_text_iter_set_offset(&speaking_end, end + start_offset);

        gtk_text_buffer_select_range(gtk_text_view_get_buffer(text_view), &speaking_start, &speaking_end);

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

    read_btn     = GTK_WIDGET (gtk_builder_get_object (builder, "read_btn"));
    pause_btn    = GTK_WIDGET (gtk_builder_get_object (builder, "pause_btn"));
    stop_btn     = GTK_WIDGET (gtk_builder_get_object (builder, "stop_btn"));
    settings_btn = GTK_WIDGET (gtk_builder_get_object (builder, "settings_btn"));
    text_view = GTK_TEXT_VIEW (gtk_builder_get_object (builder, "text_view"));
    scroll_area = GTK_SCROLLED_WINDOW (gtk_builder_get_object (builder, "scrolledwindow1"));

    if (!read_btn || !pause_btn || !text_view || !settings_btn || !scroll_area)
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
        g_object_unref(builder);
    }

    return EXIT_SUCCESS;
}
