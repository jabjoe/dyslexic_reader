#include <stdlib.h>
#include <gtkspell/gtkspell.h>
#include "reader.h"
#include "resources.h"


GtkWidget   * read_btn     = NULL;
GtkWidget   * pause_btn    = NULL;
GtkWidget   * settings_btn = NULL;
GtkWidget   * main_window  = NULL;
GtkTextView * text_view    = NULL;
GtkBuilder  * builder      = NULL;

extern void read_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    if (dyslexic_reader_start_read(reader))
    {
        gtk_text_view_set_editable (text_view, FALSE);
        gtk_widget_hide(read_btn);
        gtk_widget_show_all(pause_btn);
        gtk_widget_set_sensitive(settings_btn, FALSE);
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


extern void speed_changed_cb(GtkAdjustment *speed_spin_adj, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    dyslexic_reader_set_rate(reader, gtk_adjustment_get_value (speed_spin_adj));
}


extern void settings_btn_clicked_cb(GtkButton* btn, GtkDialog * settings_dialog )
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(main_window), "reader");
    GtkSpellChecker *spellcheck = (GtkSpellChecker*) g_object_get_data(G_OBJECT(main_window), "spellcheck");

    GtkComboBoxText* voices_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "voice_dropdown"));
    GtkComboBoxText* language_ui = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "language_dropdown"));

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

    gtk_dialog_run(settings_dialog);

    dyslexic_reader_set_voice(reader, voices[gtk_combo_box_get_active(GTK_COMBO_BOX(voices_ui))]);
    dyslexic_reader_set_language(reader, languages[gtk_combo_box_get_active(GTK_COMBO_BOX(language_ui))]);
}


void reading_stopped(dyslexic_reader_t *reader)
{
    gtk_widget_hide(pause_btn);
    gtk_widget_show_all(read_btn);
    gtk_text_view_set_editable (text_view, TRUE);
    gtk_widget_set_sensitive(settings_btn, TRUE);
}


void reading_updated(dyslexic_reader_t* reader)
{
    gtk_widget_show_all(GTK_WIDGET(text_view));
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
    GtkTextBuffer * text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (builder, "text_buffer"));
    if (!text_buffer)
    {
        g_critical("Dyslexic reader requires \"text_buffer\" from GtkBuilder.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    read_btn     = GTK_WIDGET (gtk_builder_get_object (builder, "read_btn"));
    pause_btn    = GTK_WIDGET (gtk_builder_get_object (builder, "pause_btn"));
    settings_btn = GTK_WIDGET (gtk_builder_get_object (builder, "settings_btn"));
    text_view = GTK_TEXT_VIEW (gtk_builder_get_object (builder, "text_view"));

    if (!read_btn || !pause_btn || !text_view || !settings_btn)
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


    dyslexic_reader_t* reader = dyslexic_reader_create(text_buffer);

    if (reader)
    {
        g_object_set_data(G_OBJECT(main_window), "reader", reader);
        g_object_set_data(G_OBJECT(main_window), "spellcheck", spellcheck);
        gtk_widget_show (main_window);
        gtk_main ();
        dyslexic_reader_destroy(reader);
        g_object_unref(builder);
    }

    return EXIT_SUCCESS;
}
