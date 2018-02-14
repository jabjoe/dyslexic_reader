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
#include <espeak-ng/speak_lib.h>
#include <glib.h>

#include "reader.h"


struct dyslexic_reader_t
{
    const char    * current_text;
    unsigned        current_size;
    uint32_t        paused_pos;
};

static dyslexic_reader_t * dyslexic_reader_singleton = NULL;


static void show_speaking(dyslexic_reader_t *reader, uint start, uint end)
{
    reading_updated(reader, start, end);
    reader->paused_pos = start;
}


static void dyslexic_reader_stopping(dyslexic_reader_t* reader)
{
    reader->paused_pos = -1;
    reading_stopped(reader);
}


static int _dyslexic_reader_espeak_callback(short* wav, int numsamples, espeak_EVENT* event)
{
    while(event->type)
    {
        if (event->type == espeakEVENT_WORD)
        {
            show_speaking(dyslexic_reader_singleton,
                          event->text_position - 1,
                          event->text_position - 1 + event->length);
        } else if (event->type == espeakEVENT_MSG_TERMINATED)
        {
            dyslexic_reader_stopping(dyslexic_reader_singleton);
        }

        event++;
    }

    return 0;
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

    memset(reader, 0, sizeof(dyslexic_reader_t));

    reader->paused_pos = -1;

    int r = espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, espeakEVENT_WORD | espeakEVENT_MSG_TERMINATED);
    if (r < 0)
    {
        g_critical("Dyslexic reader failed to start espeak-ng.");
        free(reader);
        return NULL;
    }

    espeak_SetSynthCallback(_dyslexic_reader_espeak_callback);

    dyslexic_reader_singleton = reader;

    return reader;
}


void dyslexic_reader_destroy(dyslexic_reader_t* reader)
{
    espeak_Terminate();
    free(reader);
}


bool dyslexic_reader_start_read(dyslexic_reader_t* reader, const char* text_start, const char* text_end)
{
    reader->paused_pos = -1;

    reader->current_text = text_start;
    reader->current_size = (unsigned)(((intptr_t)text_end) - ((uintptr_t)text_start));

    espeak_ERROR e = espeak_Synth(reader->current_text, reader->current_size,
                                  0, POS_SENTENCE, 0, espeakCHARS_AUTO, NULL, NULL);

    if (e == EE_OK)
        return true;

    return false;
}


bool dyslexic_reader_start_pause(dyslexic_reader_t* reader)
{
    return (espeak_Cancel() == EE_OK);
}


bool                dyslexic_reader_is_paused(dyslexic_reader_t* reader)
{
    return reader->paused_pos != -1;
}


bool dyslexic_reader_continue(dyslexic_reader_t* reader)
{
    espeak_ERROR e = espeak_Synth(reader->current_text, reader->current_size,
                                  reader->paused_pos, POS_SENTENCE, 0, espeakCHARS_AUTO, NULL, NULL);

    if (e == EE_OK)
        return true;

    return false;
}


bool                dyslexic_reader_start_stop(dyslexic_reader_t* reader)
{
    if (espeak_Cancel() == EE_OK)
    {
        dyslexic_reader_stopping(reader);
        return true;
    }
    return false;
}


void                dyslexic_reader_set_rate(dyslexic_reader_t* reader, double rate)
{
    espeak_SetParameter(espeakRATE, 450 * rate / 100, 0);
}


const char* const*  dyslexic_reader_list_voices(dyslexic_reader_t* reader)
{
    return NULL;
}


bool                dyslexic_reader_set_voice(dyslexic_reader_t* reader, const char* voice)
{
    return false;
}


const char* const*  dyslexic_reader_list_languages(dyslexic_reader_t* reader)
{
    return NULL;
}


const char* const*  dyslexic_reader_list_languages_short(dyslexic_reader_t* reader)
{
    return NULL;
}


bool                dyslexic_reader_set_language(dyslexic_reader_t* reader, const char* language)
{
    return false;
}
