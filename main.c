#include <stdlib.h>
#include <gtkspell/gtkspell.h>
#include <glib-unix.h>
#include "reader.h"
#include "resources.h"


GtkWidget   * read_btn     = NULL;
GtkWidget   * pause_btn    = NULL;
GtkWidget   * settings_btn = NULL;
GtkWidget   * main_window  = NULL;
GtkTextView * text_view    = NULL;
GtkBuilder  * builder      = NULL;
GtkTextBuffer     * text_buffer = NULL;
GtkTextTag        * speaking_tag;
GtkScrolledWindow * scroll_area = NULL;

GtkTextIter     speaking_start;
GtkTextIter     speaking_end;


static int pipe_ipc[2];



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

        gtk_text_buffer_get_iter_at_offset (text_buffer, &start, 0);
        gtk_text_buffer_get_iter_at_offset (text_buffer, &end, -1);

        const char* text_start = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
        const char* text_end = text_start + gtk_text_iter_get_offset(&end);

        if (dyslexic_reader_start_read(reader, text_start, text_end))
        {
            gtk_text_buffer_get_iter_at_offset(text_buffer, &speaking_start, 0);
            gtk_text_buffer_get_iter_at_offset(text_buffer, &speaking_end, 0);
            gtk_text_view_set_editable (text_view, FALSE);
            gtk_widget_hide(read_btn);
            gtk_widget_show_all(pause_btn);
            gtk_widget_set_sensitive(settings_btn, FALSE);
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


extern void settings_btn_clicked_cb(GtkButton* btn, GtkDialog * settings_dialog )
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(main_window), "reader");
    GtkSpellChecker *spellcheck = (GtkSpellChecker*) g_object_get_data(G_OBJECT(main_window), "spellcheck");

    GtkComboBoxText* voices_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "voice_dropdown"));
    GtkComboBoxText* language_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "language_dropdown"));
    GtkAdjustment* speed_spin_adj = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "speed_spin_adj"));

    gtk_combo_box_text_remove_all(voices_ui);
    gtk_combo_box_text_remove_all(language_ui);

    const char* const * voices = dyslexic_reader_list_voices(reader);
    const char* const * p = voices;

    while(*p)
        gtk_combo_box_text_append_text(voices_ui,  *p++);

    const char* const * languages = dyslexic_reader_list_languages(reader);
    p = languages;

    while(*p)
        gtk_combo_box_text_append_text(language_ui,  *p++);

    gtk_combo_box_set_active(GTK_COMBO_BOX(voices_ui), 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(language_ui), 10);

    gint result = gtk_dialog_run(settings_dialog);

    gtk_widget_hide (GTK_WIDGET(settings_dialog));

    if (result == GTK_RESPONSE_OK)
    {
        const char* voice = voices[gtk_combo_box_get_active(GTK_COMBO_BOX(voices_ui))];
        const char* language = languages[gtk_combo_box_get_active(GTK_COMBO_BOX(language_ui))];
        double rate = gtk_adjustment_get_value (speed_spin_adj);

        printf("Setting to \"%s\" in \"%s\" at %f\n", voice, language, rate);

        dyslexic_reader_set_rate(reader, rate);
        dyslexic_reader_set_voice(reader, voice);
        dyslexic_reader_set_language(reader, language);
    }
}


void reading_stopped(dyslexic_reader_t *reader)
{
    uint start = -1;
    uint end = -1;

    write(pipe_ipc[1], &start, sizeof(start));
    write(pipe_ipc[1], &end, sizeof(end));
}


void reading_updated(dyslexic_reader_t* reader, uint start, uint end)
{
    write(pipe_ipc[1], &start, sizeof(start));
    write(pipe_ipc[1], &end, sizeof(end));
}


gboolean ipc_pipe_update_cb(gint fd,
                          GIOCondition condition,
                          gpointer user_data)
{
    dyslexic_reader_t* reader = (dyslexic_reader_t*)user_data;
    uint start, end;

    read(fd, &start, sizeof(start));
    read(fd, &end, sizeof(end));

    if (start == -1 && end == -1)
    {
        gtk_text_buffer_remove_tag(text_buffer,
                               speaking_tag,
                               &speaking_start,
                               &speaking_end);

        gtk_widget_hide(pause_btn);
        gtk_widget_show_all(read_btn);
        gtk_text_view_set_editable (text_view, TRUE);
        gtk_widget_set_sensitive(settings_btn, TRUE);
    }
    else
    {
        gtk_text_buffer_remove_tag(text_buffer,
                                   speaking_tag,
                                   &speaking_start,
                                   &speaking_end);

        gtk_text_iter_set_offset(&speaking_start, start);
        gtk_text_iter_set_offset(&speaking_end, end);

        gtk_text_buffer_apply_tag(text_buffer,
                                  speaking_tag,
                                  &speaking_start,
                                  &speaking_end);

        GtkTextIter iter;

        gtk_text_buffer_get_iter_at_offset(gtk_text_view_get_buffer(text_view), &iter, end);

        gtk_text_view_scroll_to_iter(text_view, &iter, 0, FALSE, 0, 0);

        gtk_widget_show_all(GTK_WIDGET(text_view));
    }
}


int main(int argc, char* argv[])
{
    gtk_init (&argc, &argv);

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

    speaking_tag = gtk_text_buffer_create_tag(text_buffer, "speaking", "background", "black", "foreground", "white", NULL);
    if (!speaking_tag)
    {
        g_critical("Dyslexic reader failed to create speaking tag.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    read_btn     = GTK_WIDGET (gtk_builder_get_object (builder, "read_btn"));
    pause_btn    = GTK_WIDGET (gtk_builder_get_object (builder, "pause_btn"));
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
        gtk_widget_show (main_window);
        gtk_main ();
        dyslexic_reader_destroy(reader);
        g_object_unref(builder);
    }

    return EXIT_SUCCESS;
}
