
CC=gcc
CFLAGS += $(INCLUDES) -O -Wall -std=gnu99 `xml2-config --cflags`


all: MB MB_test

maestroTest: maestroTest.o
	gcc -o maestroTest maestroTest.o -I.

MB: MB.o arduino-serial/arduino-serial-lib.o
	$(CC) $(CFLAGS) -o MB MB.o arduino-serial/arduino-serial-lib.o `xml2-config --libs` -I. -I/usr/include/libxml2

MB_test: MB_test.o mbdriver.o arduino-serial-lib.o
	g++ $(CFLAGS) -o MB_test MB_test.o mbdriver.o arduino-serial-lib.o `xml2-config --libs` -I. -I/usr/include/libxml2

.c.o:
	gcc -c $*.c -o $*.o -I/usr/include/libxml2

clean:
	rm -f $(OBJ) MB *.o *.a
	rm -f $(OBJ) MB_test *.o *.a
	rm -f $(OBJ) maestroTest *.o *.a
	rm -f $(OBJ) arduino-serial/arduino-serial-lib *.o *.a
