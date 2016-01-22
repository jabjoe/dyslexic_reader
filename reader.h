#ifndef __DYSLEXIC_READER__
#define __DYSLEXIC_READER__

#include <gtk/gtk.h>

typedef struct dyslexic_reader_t dyslexic_reader_t;

extern dyslexic_reader_t * dyslexic_reader_create(GtkWidget * window, GtkTextBuffer * text_buffer);
extern void                dyslexic_reader_destroy(dyslexic_reader_t* reader);
extern void                dyslexic_reader_start_read(dyslexic_reader_t* reader);
extern void                dyslexic_reader_start_pause(dyslexic_reader_t* reader);

#endif //__DYSLEXIC_READER__
