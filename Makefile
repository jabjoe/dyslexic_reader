CFLAGS := `pkg-config gtk+-3.0 espeak-ng gtksourceview-3.0 gtkspell3-3.0 --cflags` -MMD -MP -Wall
LDFLAGS :=  -Wl,--no-as-needed  `pkg-config gtk+-3.0 espeak-ng gtksourceview-3.0 gtkspell3-3.0 --libs` -rdynamic

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

%.o.dbg : %.c Makefile
	$(CC) $(CFLAGS) -g -O0 -c $< -o $@

resources.h : resources.xml Makefile reader.svg
	$(GLIB_COMPILE_RESOURCES) --generate-header resources.xml 

resources.c : resources.xml ui.glade Makefile
	$(GLIB_COMPILE_RESOURCES) --target=resources.c --generate-source resources.xml

dyslexicreader.gschema.valid: dyslexicreader.gschema.xml
	$(GLIB_COMPILE_SCHEMAS) --strict --dry-run --schema-file=$< && mkdir -p $(@D) && touch $@

gschemas.compiled: dyslexicreader.gschema.valid
	$(GLIB_COMPILE_SCHEMAS) .

$(EXE_NAME).dbg : main.o.dbg reader.o.dbg resources.o
	$(CC) $^ -g $(LDFLAGS) -o $@

debug : $(EXE_NAME).dbg
	gdb ./$(EXE_NAME).dbg

install: $(EXE_NAME)
	cp dyslexicreader.gschema.xml $(GLIB_SCHEMAS_DIR)
	cp $(EXE_NAME) /usr/local/bin/
	mkdir -p /usr/local/share/icons/hicolor/scalable/apps/
	mkdir -p /usr/local/share/icons/hicolor/48x48/apps/
	convert -verbose reader.svg -background none -resize 48x48 -extent 48x48 /usr/local/share/icons/hicolor/48x48/apps/dyslexic_reader.png
	cp reader.svg /usr/local/share/icons/hicolor/scalable/apps/dyslexic_reader.svg
	mkdir -p /usr/local/share/applications/
	cp dyslexic_reader.desktop /usr/local/share/applications/
	$(GLIB_COMPILE_SCHEMAS) $(GLIB_SCHEMAS_DIR)

uninstall:
	rm $(GLIB_SCHEMAS_DIR)/dyslexicreader.gschema.xml
	rm /usr/local/bin/$(EXE_NAME)
	rm /usr/local/share/icons/hicolor/48x48/apps/dyslexic_reader.png
	rm /usr/local/share/icons/hicolor/scalable/apps/dyslexic_reader.svg
	rm /usr/local/share/applications/dyslexic_reader.desktop
	$(GLIB_COMPILE_SCHEMAS) $(GLIB_SCHEMAS_DIR)

.PHONT: clean
clean:
	rm -f *.[od] $(EXE_NAME)
	rm -f dyslexicreader.gschema.valid
	rm -f gschemas.compiled
	rm -rf resources.[ch]

-include *.d
