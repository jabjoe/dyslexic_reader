CFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --cflags` -MMD -MP
LDFLAGS := `pkg-config gtk+-3.0 speech-dispatcher gtkspell3-3.0 --libs` -rdynamic

EXE_NAME := dyslexic_reader

$(EXE_NAME) : main.o reader.o
	$(CC) $^ $(LDFLAGS) -o $@

.PHONT: clean
clean:
	rm -f *.[od] $(EXE_NAME)

-include *.d
