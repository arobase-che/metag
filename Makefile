

CC=gcc
PROG=metag
LIBS=-lcurses -ltag_c -lreadline
HEADERS=$(wildcard *.h)
FILES=basic_curses.c item.c readline.c regex.c wind.c
OBJ_FILES := $(FILES:.c=.o)
CFLAGS+=-std=gnu99 


all: $(PROG)

$(PROG): main.c main.h $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LIBS) -o $@ main.c $(OBJ_FILES)




%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $<



.PHONY: clean mrproper

clean:
	rm -f $(OBJ_FILES)

mrproper: clean
	rm -f ./$(PROG)


install: $(PROG)
	cp $(PROG) /usr/bin/$(PROG)
	@echo 'Installation success'

uninstall: /usr/bin/$(PROG)
	rm -f /usr/bin/$(PROG)

