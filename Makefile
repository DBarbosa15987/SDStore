#Esta Makefile é apenas provisória, depois usamos a do prof

all: clean fifo sdstore sdstored

fifo:
	mkdir FIFOs

sdstore: 
	gcc src/sdstore.c -o src/sdstore -Wall

sdstored:
	gcc src/sdstored.c -o src/sdstored -Wall

clean:
	rm -rf src/*sdstore src/*sdstored src/*.o FIFOs out/*
