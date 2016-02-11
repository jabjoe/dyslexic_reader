/*
Copyright (C) 2016  Joe Burmeister

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
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
    char         ** langs_short;
};

static dyslexic_reader_t * dyslexic_reader_singleton = NULL;


static void mark_up_text(dyslexic_reader_t *reader, const char* text_start, const char* text_end)
{
    g_string_truncate(reader->marked_text, 0);

    const char* text = text_start;
    uint32_t pos = 0;

    while(text < text_end)
    {
        const char* word_start = text;
        uint32_t word_start_pos = pos; //To be UTF-8 safe, we must count characters separate from bytes.
        gunichar c = g_utf8_get_char(word_start);

        while(word_start < text_end && (g_unichar_isspace(c) ||
                                        g_unichar_ispunct(c) ||
                                        g_unichar_iscntrl(c)))
        {
            word_start = g_utf8_next_char(word_start);
            c = g_utf8_get_char(word_start);
            ++word_start_pos;
        }

        if (word_start < text_end)
        {
            const char* word_end = word_start;
            uint32_t    word_end_pos = word_start_pos;
            c = g_utf8_get_char(word_end);

            while(word_end < text_end && (g_unichar_isalnum(c) ||
                                          g_unichar_ispunct(c)))
            {
                word_end = g_utf8_next_char(word_end);
                c = g_utf8_get_char(word_end);
                ++word_end_pos;
            }

            if (word_end == word_start)
            {
                text = g_utf8_next_char(text);
                ++pos;
                continue;
            }

            uint marked_pos = (uint)reader->marked_text->len;

            g_string_append_printf(reader->marked_text, "<mark name=\"%u-%u-%u\"/>", word_start_pos, word_end_pos, marked_pos);
            g_string_append_printf(reader->marked_text, "%.*s", (int)((uintptr_t)(word_end) - ((uintptr_t)word_start)), word_start);
            text = word_end;
            pos = word_end_pos;
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
    reader->langs_short = NULL;

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
    if (reader->langs_short)
    {
        char ** p = reader->langs_short;
        while(*p)
            free(*p++);
        free(reader->langs_short);
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
        if (!strcmp(*voices, voice))
            if (!spd_set_voice_type(reader->speech_con, type))
                return true;
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
    reader->langs_short = (char**)malloc(sizeof(char*)*count);
    char** languages_pos2 = reader->languages;
    char** langs_short_pos = reader->langs_short;
    languages_pos = languages;

    for(;*languages_pos;languages_pos++)
    {
        *languages_pos2++ = strdup((*languages_pos)->name);
        *langs_short_pos++ = strdup((*languages_pos)->language);
    }

    *languages_pos2 = NULL;
    *langs_short_pos = NULL;
    return (const char* const*)reader->languages;
}


const char* const*  dyslexic_reader_list_languages_short(dyslexic_reader_t* reader)
{
    dyslexic_reader_list_languages(reader);
    return (const char* const*)reader->langs_short;
}


bool                dyslexic_reader_set_language(dyslexic_reader_t* reader, const char* language)
{
    SPDVoice** languages = spd_list_synthesis_voices(reader->speech_con);
    SPDVoice** languages_pos = languages;

    for(;*languages_pos;languages_pos++)
        if (!strcmp((*languages_pos)->name, language) || !strcmp((*languages_pos)->language, language))
            if (!spd_set_language(reader->speech_con, (*languages_pos)->language))
                return true;
    return false;
}
