CFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --cflags` -MMD -MP -Wall
LDFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --libs` -rdynamic

GLIB_COMPILE_RESOURCES := `pkg-config --variable=glib_compile_resources gio-2.0`

EXE_NAME := dyslexic_reader

$(EXE_NAME) : main.o reader.o resources.o
	$(CC) $^ $(LDFLAGS) -o $@

*.o: Makefile

resources.h : resources.xml
	$(GLIB_COMPILE_RESOURCES) --generate-header resources.xml 

resources.c : resources.xml ui.glade
	$(GLIB_COMPILE_RESOURCES) --target=resources.c --generate-source resources.xml

.PHONT: clean
clean:
	rm -f *.[od] $(EXE_NAME)

-include *.d
