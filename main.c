#include <gtk/gtk.h>
#include <libspeechd.h>

extern void on_window_destroy_cb (GObject *object, gpointer user_data)
{
        gtk_main_quit();
}


extern void read_btn_clicked_cb (GObject *object, gpointer user_data)
{
    SPDConnection *speech_con = (SPDConnection*) g_object_get_data(G_OBJECT(user_data), "speech_con");

    spd_say(speech_con, SPD_TEXT, "hello world");
}


extern void stop_btn_clicked_cb (GObject *object, gpointer user_data)
{
    SPDConnection *speech_con = (SPDConnection*) g_object_get_data(G_OBJECT(user_data), "speech_con");
}



int main(int argc, char* argv[])
{
    gtk_init (&argc, &argv);

    SPDConnection *speech_con = spd_open("dyslexic_reader", NULL, NULL, SPD_MODE_THREADED);

    if (speech_con)
    {
        GtkBuilder              *builder;
        GtkWidget               *window;

        builder = gtk_builder_new ();
        gtk_builder_add_from_file (builder, "ui.glade", NULL);

        window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));

        g_object_set_data(G_OBJECT(window), "speech_con", speech_con);

        gtk_builder_connect_signals (builder, NULL);          
        g_object_unref (G_OBJECT (builder));

        gtk_widget_show (window);
        gtk_main ();

        spd_close(speech_con);
    }

    return 0;
}
