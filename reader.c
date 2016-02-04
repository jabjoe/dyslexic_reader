#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libspeechd.h>

#include "reader.h"


struct dyslexic_reader_t
{
    GString       * marked_text;
    SPDConnection * speech_con;
    GtkTextBuffer * text_buffer;
    uint32_t        paused_pos;
    void          * userdata;
    GtkTextIter     speaking_start;
    GtkTextIter     speaking_end;
    GtkTextTag    * speaking_tag;
    char         ** languages;
};

static dyslexic_reader_t * dyslexic_reader_singleton = NULL;


static void mark_up_text(dyslexic_reader_t *reader)
{
    char mark_buf[32];
    GtkTextIter start, end;

    g_string_truncate(reader->marked_text, 0);

    gtk_text_buffer_get_iter_at_offset (reader->text_buffer, &start, 0);

    do
    {
        gtk_text_iter_assign(&end, &start);

        if (!gtk_text_iter_forward_word_end(&end))
            gtk_text_iter_forward_to_end(&end);

        uint32_t marked_pos = reader->marked_text->len;

        g_string_append_printf(reader->marked_text, "<mark name=\"%i-%i-%u\"/>", gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(&end), marked_pos);
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
}


static void show_speaking(dyslexic_reader_t *reader, gint start, gint end, uint32_t mark_pos)
{
    gtk_text_buffer_remove_tag(reader->text_buffer,
                               reader->speaking_tag,
                               &reader->speaking_start,
                               &reader->speaking_end);

    gtk_text_iter_set_offset(&reader->speaking_start, start);
    gtk_text_iter_set_offset(&reader->speaking_end, end);

    gtk_text_buffer_apply_tag(reader->text_buffer,
                              reader->speaking_tag,
                              &reader->speaking_start,
                              &reader->speaking_end);

    reading_updated(reader, &reader->speaking_start);
    reader->paused_pos = mark_pos;
}



static void speech_im_cb(size_t msg_id, size_t client_id, SPDNotificationType state, char *index_mark)
{
    if (index_mark)
    {
        gint i,j;
        uint32_t mark_pos;
        printf("index_mark: %s\n", index_mark);
        sscanf(index_mark, "%i-%i-%u", &i, &j, &mark_pos);
        show_speaking(dyslexic_reader_singleton, i, j, mark_pos);
    }
}

static void dyslexic_reader_stopping(dyslexic_reader_t* reader)
{
    reader->paused_pos = -1;
    gtk_text_buffer_remove_tag(reader->text_buffer,
                               reader->speaking_tag,
                               &reader->speaking_start,
                               &reader->speaking_end);
    reading_stopped(reader);
}


static void speech_end_cb(size_t msg_id, size_t client_id, SPDNotificationType state)
{
    dyslexic_reader_stopping(dyslexic_reader_singleton);
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
    reader->paused_pos = -1;
    reader->userdata = NULL;
    reader->languages = NULL;

    reader->speech_con  = spd_open("dyslexic_reader", NULL, NULL, SPD_MODE_THREADED);
    if (!reader->speech_con)
    {
        g_critical("Dyslexic reader failed to open a SDP connection.");
        free(reader);
        return NULL;
    }

    reader->speaking_tag = gtk_text_buffer_create_tag(reader->text_buffer, "speaking", "background", "black", "foreground", "white", NULL);
    if (!reader->speaking_tag)
    {
        g_critical("Dyslexic reader failed to create speaking tag.");
        spd_close(reader->speech_con);
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
   if (reader->languages)
   {
       char ** p = reader->languages;
       while(*p)
           free(*p++);
       free(reader->languages);
   }
   free(reader);
}


bool dyslexic_reader_start_read(dyslexic_reader_t* reader)
{
    if (reader->paused_pos != -1)
    {
        int msg_id = spd_say(reader->speech_con, SPD_TEXT, reader->marked_text->str + reader->paused_pos);
        printf("msg_id:%i (pos:%u)\n", msg_id, reader->paused_pos);
        if (msg_id > 0)
            return true;
    }
    else
    {
        mark_up_text(reader);
        if (reader->marked_text->len)
        {
            gtk_text_buffer_get_iter_at_offset(reader->text_buffer, &reader->speaking_start, 0);
            gtk_text_buffer_get_iter_at_offset(reader->text_buffer, &reader->speaking_end, 0);

            int msg_id = spd_say(reader->speech_con, SPD_TEXT, reader->marked_text->str);
            printf("msg_id:%i\n", msg_id);
            if (msg_id > 0)
                return true;
        }
        return false;
    }
}


bool dyslexic_reader_start_pause(dyslexic_reader_t* reader)
{
    return (!spd_stop(reader->speech_con));
}


bool                dyslexic_reader_start_stop(dyslexic_reader_t* reader)
{
    if (spd_stop(reader->speech_con) == 0)
    {
        dyslexic_reader_stopping(reader);
        return true;
    }
    return false;
}


void                dyslexic_reader_set_rate(dyslexic_reader_t* reader, double rate)
{
    spd_set_voice_rate(reader->speech_con, rate);
}


const char* const*  dyslexic_reader_list_voices(dyslexic_reader_t* reader)
{
    return (const char* const*)spd_list_voices(reader->speech_con);
}


bool                dyslexic_reader_set_voice(dyslexic_reader_t* reader, const char* voice)
{
    const char* const* voices = (const char* const*)spd_list_voices(reader->speech_con);
    SPDVoiceType type = SPD_MALE1;
    while(*voices)
    {
        if (*voices == voice)
        {
            if (!spd_set_voice_type(reader->speech_con, type))
                return true;
        }
        voices++;
        type++;
    }
    return false;
}


const char* const*  dyslexic_reader_list_languages(dyslexic_reader_t* reader)
{
    if (reader->languages)
        return (const char* const*)reader->languages;

    SPDVoice** languages = spd_list_synthesis_voices(reader->speech_con);
    SPDVoice** languages_pos = languages;
    uint32_t count = 1; //For null

    while(*languages_pos++)
        count++;

    reader->languages = (char**)malloc(sizeof(char*)*count);
    char** languages_pos2 = reader->languages;
    languages_pos = languages;

    for(;*languages_pos;languages_pos++)
        *languages_pos2++ = strdup((*languages_pos)->name);

    *languages_pos2 = NULL;
    return (const char* const*)reader->languages;
}


bool                dyslexic_reader_set_language(dyslexic_reader_t* reader, const char* language)
{
    SPDVoice** languages = spd_list_synthesis_voices(reader->speech_con);
    SPDVoice** languages_pos = languages;

    for(;*languages_pos;languages_pos++)
        if (!strcmp((*languages_pos)->name, language))
            if (!spd_set_language(reader->speech_con, (*languages_pos)->language))
                return true;
    return false;
}
