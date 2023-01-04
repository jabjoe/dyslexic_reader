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
    char              * current_text;
    unsigned            current_size;
    double              rate;
};

static dyslexic_reader_t * dyslexic_reader_singleton = NULL;


static void show_speaking(dyslexic_reader_t *reader, uint start, uint end)
{
    reading_updated(reader, start, end);
    reading_debug("Reading at %u", start);
}


static void dyslexic_reader_stopping(dyslexic_reader_t* reader)
{
    reading_stopped(reader);
    reading_debug("Reading stopped");
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
            reading_debug("Finished reading.");
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

    dyslexic_reader_singleton = reader;
    reader->rate = 100;

    return reader;
}


void dyslexic_reader_destroy(dyslexic_reader_t* reader)
{
    espeak_Terminate();
    free(reader);
}


bool dyslexic_reader_finsihed(void)
{
    espeak_Synchronize();
    espeak_Terminate();
    if (dyslexic_reader_singleton->current_text)
    {
        free(dyslexic_reader_singleton->current_text);
        dyslexic_reader_singleton->current_text = NULL;
    }
}


bool dyslexic_reader_start_read(dyslexic_reader_t* reader, const char* text_start, const char* text_end)
{
    if (reader->current_text)
    {
        free(reader->current_text);
        reader->current_text = NULL;
    }

    reader->current_size = (unsigned)(((intptr_t)text_end) - ((uintptr_t)text_start)) + 1;
    reader->current_text = malloc(reader->current_size );

    memcpy(reader->current_text, text_start, reader->current_size);
    reader->current_text[reader->current_size] = 0;

    reading_debug("Beginning reading");

    int r = espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, espeakINITIALIZE_DONT_EXIT);
    if (r < 0)
    {
        g_critical("Dyslexic reader failed to start espeak-ng.");
        return NULL;
    }

    espeak_SetSynthCallback(_dyslexic_reader_espeak_callback);
    espeak_SetParameter(espeakRATE, 450 * reader->rate / 100, 0);

    espeak_ERROR e = espeak_Synth(reader->current_text, reader->current_size,
                                  0, POS_WORD, reader->current_size, espeakCHARS_AUTO, NULL, NULL);

    if (e == EE_OK)
        return true;

    reading_debug("Failed to start read.");

    return false;
}


bool                dyslexic_reader_is_reading(dyslexic_reader_t* reader)
{
    return (reader->current_text)?true:false;
}


bool                dyslexic_reader_start_stop(dyslexic_reader_t* reader)
{
    reading_debug("Forcing reading stop.");
    if (espeak_Cancel() == EE_OK)
        return true;

    reading_debug("Failed to stop.");
    return false;
}


void                dyslexic_reader_set_rate(dyslexic_reader_t* reader, double rate)
{
    reading_debug("Set rate to %G", rate);
    reader->rate = rate;
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
