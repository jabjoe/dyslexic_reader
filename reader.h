#ifndef __DYSLEXIC_READER__
#define __DYSLEXIC_READER__

#include <stdbool.h>
#include <gtk/gtk.h>

typedef struct dyslexic_reader_t dyslexic_reader_t;

extern dyslexic_reader_t * dyslexic_reader_create(GtkTextBuffer * text_buffer);
extern void                dyslexic_reader_destroy(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_start_read(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_start_pause(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_is_paused(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_continue(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_start_stop(dyslexic_reader_t* reader);

extern void                dyslexic_reader_set_rate(dyslexic_reader_t* reader, double rate);

extern const char* const*  dyslexic_reader_list_voices(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_set_voice(dyslexic_reader_t* reader, const char* voice);

extern const char* const*  dyslexic_reader_list_languages(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_set_language(dyslexic_reader_t* reader, const char* language);

//Externally defined.
extern void                reading_stopped(dyslexic_reader_t* reader);
extern void                reading_updated(dyslexic_reader_t* reader, uint start, uint end);


#endif //__DYSLEXIC_READER__
