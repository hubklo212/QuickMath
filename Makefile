CC=gcc
CFLAGS=-I. -lsqlite3
#CFLAGS=

OBJECTS = server client

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
