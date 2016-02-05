#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <libspeechd.h>
#include <glib.h>

#include "reader.h"


struct dyslexic_reader_t
{
    GString       * marked_text;
    SPDConnection * speech_con;
    uint32_t        paused_pos;
    void          * userdata;
    char         ** languages;
};

static dyslexic_reader_t * dyslexic_reader_singleton = NULL;


static void mark_up_text(dyslexic_reader_t *reader, const char* text_start, const char* text_end)
{
    g_string_truncate(reader->marked_text, 0);

    const char* text = text_start;

    while(text < text_end)
    {
        const char* word_start = text;

        while(word_start < text_end && (isspace(*word_start) || ispunct(*word_start) || iscntrl(*word_start)))
            word_start++;

        if (word_start < text_end)
        {
            const char* word_end = word_start;

            while(word_end < text_end && (isalnum(*word_end) || ispunct(*word_end)))
                word_end++;

            if (word_end == word_start)
            {
                text++;
                continue;
            }

            uint marked_pos = (uint)reader->marked_text->len;

            g_string_append_printf(reader->marked_text, "<mark name=\"%u-%u-%u\"/>", (uint)((uintptr_t)(word_start) - ((uintptr_t)text_start)), (uint)((uintptr_t)(word_end) - ((uintptr_t)text_start)), marked_pos);
            g_string_append_printf(reader->marked_text, "%.*s", (int)((uintptr_t)(word_end) - ((uintptr_t)word_start)), word_start);
            text = word_end;
        }
        else break;
    }
}


static void show_speaking(dyslexic_reader_t *reader, uint start, uint end, uint32_t mark_pos)
{
    reading_updated(reader, start, end);
    reader->paused_pos = mark_pos;
}



static void speech_im_cb(size_t msg_id, size_t client_id, SPDNotificationType state, char *index_mark)
{
    if (index_mark)
    {
        uint i,j, mark_pos;
        printf("index_mark: %s\n", index_mark);
        sscanf(index_mark, "%u-%u-%u", &i, &j, &mark_pos);
        show_speaking(dyslexic_reader_singleton, i, j, mark_pos);
    }
}

static void dyslexic_reader_stopping(dyslexic_reader_t* reader)
{
    reader->paused_pos = -1;
    reading_stopped(reader);
}


static void speech_end_cb(size_t msg_id, size_t client_id, SPDNotificationType state)
{
    dyslexic_reader_stopping(dyslexic_reader_singleton);
}


dyslexic_reader_t *dyslexic_reader_create()
{
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

    reader->speech_con->callback_im = speech_im_cb;
    reader->speech_con->callback_end = speech_end_cb;

    spd_set_notification_on(reader->speech_con, SPD_ALL);
    spd_set_data_mode(reader->speech_con, SPD_DATA_SSML);

    dyslexic_reader_singleton = reader;

    return reader;
}


void dyslexic_reader_destroy(dyslexic_reader_t* reader)
{
    spd_stop(reader->speech_con);
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


bool dyslexic_reader_start_read(dyslexic_reader_t* reader, const char* text_start, const char* text_end)
{
    reader->paused_pos = -1;
    mark_up_text(reader, text_start, text_end);
    if (reader->marked_text->len)
    {
        int msg_id = spd_say(reader->speech_con, SPD_TEXT, reader->marked_text->str);
        printf("msg_id:%i\n", msg_id);
        if (msg_id > 0)
            return true;
    }
    return false;
}


bool dyslexic_reader_start_pause(dyslexic_reader_t* reader)
{
    return (!spd_stop(reader->speech_con));
}


bool                dyslexic_reader_is_paused(dyslexic_reader_t* reader)
{
    return reader->paused_pos != -1;
}


bool dyslexic_reader_continue(dyslexic_reader_t* reader)
{
    if (reader->paused_pos == -1)
        return false;
    int msg_id = spd_say(reader->speech_con, SPD_TEXT, reader->marked_text->str + reader->paused_pos);
    printf("msg_id:%i (pos:%u)\n", msg_id, reader->paused_pos);
    return (msg_id > 0);
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
