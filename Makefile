CFLAGS := `pkg-config gtk+-3.0 speech-dispatcher --cflags` -MMD -MP
LDFLAGS := `pkg-config gtk+-3.0 speech-dispatcher --libs` -rdynamic

EXE_NAME := dyslexic_reader

$(EXE_NAME) : main.o
	$(CC) $^ $(LDFLAGS) -o $@

.PHONT: clean
clean:
	rm -f *.[od] $(EXE_NAME)

-include *.d
