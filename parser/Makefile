CC= gcc
CFLAGS = -Wall -std=c11
INCLUDE= -Iinclude -Isrc
LIB= sharedLib.so

all: $(LIB)

$(LIB): LinkedListAPI.o GEDCOMparser.o GEDCOMutilities.o
	$(CC) $(CFLAGS) -shared -o $(LIB) LinkedListAPI.o GEDCOMparser.o GEDCOMutilities.o 
	mv $(LIB) ../$(LIB)

LinkedListAPI.o: src/LinkedListAPI.c include/LinkedListAPI.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -fpic src/LinkedListAPI.c
	
GEDCOMparser.o: src/GEDCOMparser.c include/GEDCOMparser.h 
	$(CC) $(CFLAGS) $(INCLUDE) -c -fpic src/GEDCOMparser.c
	
GEDCOMutilities.o: src/GEDCOMutilities.c include/GEDCOMutilities.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -fpic src/GEDCOMutilities.c

clean:
	rm -rf *.o ../$(LIB)