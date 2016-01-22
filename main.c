#include <stdlib.h>
#include "reader.h"


GtkWidget   * read_btn  = NULL;
GtkWidget   * pause_btn = NULL;
GtkTextView * text_view = NULL;


extern void read_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    if (dyslexic_reader_start_read(reader))
    {
        gtk_text_view_set_editable (text_view, FALSE);
        gtk_widget_hide(read_btn);
        gtk_widget_show_all(pause_btn);
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


void reading_stopped(dyslexic_reader_t *reader)
{
    gtk_widget_hide(pause_btn);
    gtk_widget_show_all(read_btn);
    gtk_text_view_set_editable (text_view, TRUE);
}


int main(int argc, char* argv[])
{
    gtk_init (&argc, &argv);

    GtkBuilder    * builder = gtk_builder_new ();

    if (!builder)
    {
        g_critical("GtkBuilder object allocation failed.");
        return EXIT_FAILURE;
    }

    if (!gtk_builder_add_from_file (builder, "ui.glade", NULL))
    {
        g_critical("Dyslexic reader GtkBuilder failed to process file \"ui.glade\".");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    GtkWidget * window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    if (!window)
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

    read_btn  = GTK_WIDGET (gtk_builder_get_object (builder, "read_btn"));
    pause_btn = GTK_WIDGET (gtk_builder_get_object (builder, "pause_btn"));
    text_view = GTK_TEXT_VIEW (gtk_builder_get_object (builder, "text_view"));

    if (!read_btn || !pause_btn || !text_view)
    {
        g_critical("Dyslexic reader requires \"read_btn\" + \"pause_btn\" + \"text_view\" from GtkBuilder.");
        g_object_unref(builder);
        return EXIT_FAILURE;
    }

    gtk_builder_connect_signals (builder, NULL); 

    dyslexic_reader_t* reader = dyslexic_reader_create(text_buffer);

    if (reader)
    {
        g_object_set_data(G_OBJECT(window), "reader", reader);
        gtk_widget_show (window);
        gtk_main ();
        dyslexic_reader_destroy(reader);
        g_object_unref(builder);
    }

    return EXIT_SUCCESS;
}
