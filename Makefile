
CC=gcc
CFLAGS += $(INCLUDES) -O -Wall -std=gnu99 `xml2-config --cflags`


all: MB

maestroTest: maestroTest.o
	gcc -o maestroTest maestroTest.o -I.

MB: MB.o arduino-serial/arduino-serial-lib.o
	$(CC) $(CFLAGS) -o MB MB.o arduino-serial/arduino-serial-lib.o `xml2-config --libs` -I. -I/usr/include/libxml2

.c.o:
	gcc -c $*.c -o $*.o -I/usr/include/libxml2

clean:
	rm -f $(OBJ) MB *.o *.a
	rm -f $(OBJ) maestroTest *.o *.a
