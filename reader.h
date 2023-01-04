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

#ifndef __DYSLEXIC_READER__
#define __DYSLEXIC_READER__

#include <stdbool.h>

typedef struct dyslexic_reader_t dyslexic_reader_t;

extern dyslexic_reader_t * dyslexic_reader_create();
extern void                dyslexic_reader_destroy(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_start_read(dyslexic_reader_t* reader, const char* text_start, const char* text_end);
extern bool                dyslexic_reader_is_reading(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_start_stop(dyslexic_reader_t* reader);

extern void                dyslexic_reader_set_rate(dyslexic_reader_t* reader, double rate);

extern const char* const*  dyslexic_reader_list_voices(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_set_voice(dyslexic_reader_t* reader, const char* voice);

extern const char* const*  dyslexic_reader_list_languages(dyslexic_reader_t* reader);
extern const char* const*  dyslexic_reader_list_languages_short(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_set_language(dyslexic_reader_t* reader, const char* language);

//Externally defined.
extern void                reading_stopped(dyslexic_reader_t* reader);
extern void                reading_updated(dyslexic_reader_t* reader, uint start, uint end);
extern void                reading_debug(char * fmt, ...);


#endif //__DYSLEXIC_READER__
