CFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --cflags` -MMD -MP -Wall
LDFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --libs` -rdynamic

EXE_NAME := dyslexic_reader

$(EXE_NAME) : main.o reader.o resources.o
	$(CC) $^ $(LDFLAGS) -o $@

resources.h : resources.xml
	glib-compile-resources --generate-header resources.xml 

resources.c : resources.xml ui.glade
	glib-compile-resources --target=resources.c --generate-source resources.xml

.PHONT: clean
clean:
	rm -f *.[od] $(EXE_NAME)

-include *.d
