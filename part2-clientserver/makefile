all: server client 

server: server.o stream.o 
	gcc server.o stream.o -o server 

client: client.o stream.o 
	gcc client.o stream.o -o client 

server.o: server.c stream.h 
	gcc -c server.c 

client.o: client.c stream.h 
	gcc -c client.c 

stream.o: stream.c stream.h 
	gcc -c stream.c 

clean:
	rm -f *o server client
