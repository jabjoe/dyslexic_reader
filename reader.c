#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <libspeechd.h>

#include "reader.h"


struct dyslexic_reader_t
{
    GString       * marked_text;
    SPDConnection * speech_con;
    GtkTextBuffer * text_buffer;
    bool            paused;
    bool            reading;
    void          * userdata;
};

dyslexic_reader_t * dyslexic_reader_singleton = NULL;


static void mark_up_text(dyslexic_reader_t *reader)
{
    char mark_buf[32];
    GtkTextIter start;
    GtkTextIter end;

    g_string_truncate(reader->marked_text, 0);

    gtk_text_buffer_get_iter_at_offset (reader->text_buffer, &start, 0);

    do
    {
        gtk_text_iter_assign(&end, &start);

        if (!gtk_text_iter_forward_word_end(&end))
            gtk_text_iter_forward_to_end(&end);

        g_string_append_printf(reader->marked_text, "<mark name=\"%i-%i\"/>", gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(&end));
        g_string_append_printf(reader->marked_text, gtk_text_iter_get_visible_slice (&start, &end));

        if (!gtk_text_iter_is_end(&end))
        {
            GtkTextIter gap;
            gtk_text_iter_assign(&start, &end);
            gtk_text_iter_assign(&gap, &end);

            while(gtk_text_iter_forward_char(&start) && !gtk_text_iter_starts_word(&start));

            g_string_append_printf(reader->marked_text, gtk_text_iter_get_visible_slice (&gap, &start));
        }
    }
    while(!gtk_text_iter_is_end(&start));

    printf("Marked text: \"%s\"\n", reader->marked_text->str);
}


static void speech_im_cb(size_t msg_id, size_t client_id, SPDNotificationType state, char *index_mark)
{
    printf("state: %i\n", state);
    if (index_mark)
        printf("index_mark: %s\n", index_mark);
}


static void speech_end_cb(size_t msg_id, size_t client_id, SPDNotificationType state)
{
    dyslexic_reader_singleton->reading = false;
    dyslexic_reader_singleton->paused = false;
    reading_stopped(dyslexic_reader_singleton);
}


dyslexic_reader_t *dyslexic_reader_create(GtkTextBuffer * text_buffer)
{
    assert(text_buffer);

    if (dyslexic_reader_singleton)
    {
        g_critical("Dyslexic reader singleton already set.");
        return NULL;
    }

    dyslexic_reader_t* reader = (dyslexic_reader_t*)malloc(sizeof(dyslexic_reader_t));
    if (!reader)
    {
        g_critical("Dyslexic reader allocation failed.");
        return NULL;
    }

    reader->marked_text = g_string_new(NULL);
    if (!reader->marked_text)
    {
        g_critical("Dyslexic reader failed create GString.");
        free(reader);
        return NULL;
    }

    reader->text_buffer = text_buffer;
    reader->paused = false;
    reader->reading = false;
    reader->userdata = NULL;

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

    dyslexic_reader_singleton = reader;

    return reader;
}


void dyslexic_reader_destroy(dyslexic_reader_t* reader)
{
   spd_close(reader->speech_con);
   free(reader);
}


bool dyslexic_reader_start_read(dyslexic_reader_t* reader)
{
    if (reader->paused)
        return (spd_resume(reader->speech_con) == 0);
    else
    {
        mark_up_text(reader);
        if (reader->marked_text->len)
        {
            int msg_id = spd_say(reader->speech_con, SPD_TEXT, reader->marked_text->str);
            printf("msg_id:%i\n", msg_id);
            if (msg_id > 0)
            {
                reader->reading = true;
                return true;
            }
        }
        return false;
    }
}


bool dyslexic_reader_start_pause(dyslexic_reader_t* reader)
{
    if (spd_pause(reader->speech_con) == 0)
    {
        reader->paused = true;
        return true;
    }
    return false;
}


bool                dyslexic_reader_start_stop(dyslexic_reader_t* reader)
{
    if (spd_stop(reader->speech_con) == 0)
    {
        reader->paused = false;
        reader->reading = false;
        reading_stopped(reader);
        return true;
    }
    return false;
}


bool                dyslexic_reader_is_reading(dyslexic_reader_t* reader)
{
    return reader->reading;
}
