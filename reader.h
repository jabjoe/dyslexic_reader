#ifndef __DYSLEXIC_READER__
#define __DYSLEXIC_READER__

#include <stdbool.h>
#include <gtk/gtk.h>

typedef struct dyslexic_reader_t dyslexic_reader_t;

extern dyslexic_reader_t * dyslexic_reader_create(GtkTextBuffer * text_buffer);
extern void                dyslexic_reader_destroy(dyslexic_reader_t* reader);
extern void                dyslexic_reader_start_read(dyslexic_reader_t* reader);
extern void                dyslexic_reader_start_pause(dyslexic_reader_t* reader);
extern void                dyslexic_reader_start_stop(dyslexic_reader_t* reader);
extern bool                dyslexic_reader_is_reading(dyslexic_reader_t* reader);

extern void                dyslexic_reader_set_userdata(dyslexic_reader_t* reader, void * userdata);
extern void              * dyslexic_reader_get_userdata(dyslexic_reader_t* reader);

//Externally defined.
extern void                reading_stopped(dyslexic_reader_t* reader);


#endif //__DYSLEXIC_READER__
