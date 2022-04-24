#Esta Makefile é apenas provisória, depois usamos a do prof

all: clean fifo client server

fifo:
	mkdir FIFOs

client: 
	gcc src/client.c -o src/client -Wall

server:
	gcc src/server.c -o src/server -Wall

clean:
	rm -rf src/*client src/*server src/*.o FIFOs
