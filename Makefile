
CC=gcc
CFLAGS += $(INCLUDES) -O -Wall -std=gnu99 `xml2-config --cflags`


all: MB MB_test MB_sample

maestroTest: maestroTest.o
	gcc -o maestroTest maestroTest.o -I.

MB: MB.o arduino-serial/arduino-serial-lib.o
	$(CC) $(CFLAGS) -o MB MB.o arduino-serial/arduino-serial-lib.o `xml2-config --libs` -I. -I/usr/include/libxml2

libmbdriver.a: mbdriver.o arduino-serial-lib.o
	ar -rv $@ $^

MB_test: MB_test.o libmbdriver.a
	g++ $(CFLAGS) -o MB_test MB_test.o `xml2-config --libs` -L. -lmbdriver -I. -I/usr/include/libxml2

MB_sample: MB_sample.o libmbdriver.a
	g++ $(CFLAGS) -o MB_sample MB_sample.o `xml2-config --libs` -L. -lmbdriver -I. -I/usr/include/libxml2

.c.o:
	gcc -c $*.c -o $*.o -I/usr/include/libxml2

clean:
	rm -f $(OBJ) MB *.o *.a
	rm -f $(OBJ) MB_test *.o *.a
	rm -f $(OBJ) MB_sample *.o *.a
	rm -f $(OBJ) maestroTest *.o *.a
	rm -f $(OBJ) arduino-serial/arduino-serial-lib *.o *.a
