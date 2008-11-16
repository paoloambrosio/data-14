CC = gcc
CFLAGS = -Wall -I../include
LDFLAGS =
SOURCES = d14usr.c convert.c device.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = d14fltd

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) *~
