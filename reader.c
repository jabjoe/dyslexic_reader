#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <libspeechd.h>

#include "reader.h"


struct dyslexic_reader_t
{
    char          * marked_text;
    uint32_t        marked_text_len;
    SPDConnection * speech_con;
    GtkTextBuffer * text_buffer;
    GtkWidget     * window;
};



static void speech_im_cb(size_t msg_id, size_t client_id, SPDNotificationType state, char *index_mark)
{
    printf("state: %i\n", state);
    if (index_mark)
        printf("index_mark: %s\n", index_mark);
}


static void speech_end_cb(size_t msg_id, size_t client_id, SPDNotificationType state)
{
    
}


dyslexic_reader_t *dyslexic_reader_create(GtkWidget * window, GtkTextBuffer * text_buffer)
{
    assert(window);
    assert(text_buffer);
    
    dyslexic_reader_t* reader = (dyslexic_reader_t*)malloc(sizeof(dyslexic_reader_t));
    if (!reader)
    {
        g_critical("Dyslexic reader allocation failed.");
        return NULL;
    }

    reader->window      = window;
    reader->text_buffer = text_buffer;

    reader->speech_con  = spd_open("dyslexic_reader", NULL, NULL, SPD_MODE_THREADED);
    if (!reader->speech_con)
    {
        g_critical("Dyslexic reader failed to open a SDP connection.");
        free(reader);
        return NULL;
    }

    reader->speech_con->callback_im = speech_im_cb;
    reader->speech_con->callback_end = speech_end_cb;

    spd_set_notification_on(reader->speech_con, SPD_ALL);
    spd_set_data_mode(reader->speech_con, SPD_DATA_SSML);

    g_object_set_data(G_OBJECT(reader->window), "reader", reader);

    return reader;
}


void dyslexic_reader_destroy(dyslexic_reader_t* reader)
{
   spd_close(reader->speech_con);
   free(reader);
}


void dyslexic_reader_start_read(dyslexic_reader_t* reader)
{
    spd_say(reader->speech_con, SPD_TEXT, "<mark name=\"id\"/>hello <mark name=\"id2\"/>world");
}


void dyslexic_reader_start_pause(dyslexic_reader_t* reader)
{
    spd_pause(reader->speech_con);
}
