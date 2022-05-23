#Esta Makefile é apenas provisória, depois usamos a do prof

# all: clean fifo sdstore sdstored

# fifo:
# 	mkdir FIFOs

# sdstore: 
# 	gcc src/sdstore.c -o src/sdstore -Wall

# sdstored:
# 	gcc src/sdstored.c -o src/sdstored -Wall

# clean:
# 	rm -rf src/*sdstore src/*sdstored src/*.o FIFOs out/*

all: server client

server: bin/sdstored

client: bin/sdstore

bin/sdstored: obj/sdstored.o
	gcc -g obj/sdstored.o -o bin/sdstored

obj/sdstored.o: src/sdstored.c
	gcc -Wall -g -c src/sdstored.c -o obj/sdstored.o

bin/sdstore: obj/sdstore.o
	gcc -g obj/sdstore.o -o bin/sdstore

obj/sdstore.o: src/sdstore.c
	gcc -Wall -g -c src/sdstore.c -o obj/sdstore.o

clean: 
	rm obj/* FIFOs/* bin/{sdstore,sdstored}