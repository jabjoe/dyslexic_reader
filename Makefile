CFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --cflags` -MMD -MP -Wall
LDFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --libs` -rdynamic

GLIB_COMPILE_RESOURCES := `pkg-config --variable=glib_compile_resources gio-2.0`
GLIB_COMPILE_SCHEMAS := `pkg-config --variable=glib_compile_schemas gio-2.0`

GLIB_SCHEMAS_DIR := /usr/share/glib-2.0/schemas/

EXE_NAME := dyslexic_reader
EXE_OBJS := main.o reader.o resources.o

$(EXE_NAME) : $(EXE_OBJS) gschemas.compiled
	$(CC) $(EXE_OBJS) $(LDFLAGS) -o $@

*.o: Makefile

main.o : resources.h
resources.o : resources.c

resources.h : resources.xml Makefile reader.svg
	$(GLIB_COMPILE_RESOURCES) --generate-header resources.xml 

resources.c : resources.xml ui.glade Makefile
	$(GLIB_COMPILE_RESOURCES) --target=resources.c --generate-source resources.xml

dyslexicreader.gschema.valid: dyslexicreader.gschema.xml
	$(GLIB_COMPILE_SCHEMAS) --strict --dry-run --schema-file=$< && mkdir -p $(@D) && touch $@

gschemas.compiled: dyslexicreader.gschema.valid
	$(GLIB_COMPILE_SCHEMAS) .

install: $(EXE_NAME)
	cp dyslexicreader.gschema.xml $(GLIB_SCHEMAS_DIR)
	cp $(EXE_NAME) /usr/local/bin/
	$(GLIB_COMPILE_SCHEMAS) $(GLIB_SCHEMAS_DIR)

uninstall:
	rm $(GLIB_SCHEMAS_DIR)/dyslexicreader.gschema.xml
	rm /usr/local/bin/$(EXE_NAME)
	$(GLIB_COMPILE_SCHEMAS) $(GLIB_SCHEMAS_DIR)

.PHONT: clean
clean:
	rm -f *.[od] $(EXE_NAME)
	rm -f dyslexicreader.gschema.valid
	rm -f gschemas.compiled

-include *.d
