#include <stdlib.h>
#include "reader.h"


extern void read_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");
    GtkButton *button = GTK_BUTTON(object);

    dyslexic_reader_set_userdata(reader, button);

    if (!dyslexic_reader_is_reading(reader))
    {
        gtk_button_set_label(button, "Pause");
        dyslexic_reader_start_read(reader);
    }
    else
    {
        gtk_button_set_label(button, "Read");
        dyslexic_reader_start_pause(reader);
    }
}


extern void stop_btn_clicked_cb (GObject *object, gpointer user_data)
{
    dyslexic_reader_t *reader = (dyslexic_reader_t*) g_object_get_data(G_OBJECT(user_data), "reader");

    dyslexic_reader_start_stop(reader);
}


void reading_stopped(dyslexic_reader_t *reader)
{
    GtkButton *button = (GtkButton*)dyslexic_reader_get_userdata(reader);
    if (button)
        gtk_button_set_label(button, "Read");
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
