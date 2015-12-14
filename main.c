#include <gtk/gtk.h>
#include <libspeechd.h>

extern void on_window_destroy (GObject *object, gpointer user_data)
{
        gtk_main_quit();
}


int main(int argc, char* argv[])
{
    gtk_init (&argc, &argv);

    SPDConnection *speech_con = spd_open("dyslexic_reader", NULL, NULL, SPD_MODE_THREADED);

    if (speech_con)
    {
        GtkBuilder              *builder;
        GtkWidget               *window;

        spd_say(speech_con, SPD_TEXT, "hello world");

        builder = gtk_builder_new ();
        gtk_builder_add_from_file (builder, "ui.glade", NULL);

        window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
        gtk_builder_connect_signals (builder, NULL);          
        g_object_unref (G_OBJECT (builder));

        gtk_widget_show (window);
        gtk_main ();
        
        spd_close(speech_con);
    }

    return 0;
}
