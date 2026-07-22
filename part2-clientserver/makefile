all: server client 

server: server.o stream.o 
	gcc server.o stream.o -o server 

client: client.o stream.o client_io.o history.o 
	gcc client.o stream.o client_io.o history.o -o client 

server.o: server.c stream.h 
	gcc -c server.c 

client.o: client.c stream.h client_io.h history.h 
	gcc -c client.c 

stream.o: stream.c stream.h 
	gcc -c stream.c 

client_io.o: client_io.c client_io.h 
	gcc -c client_io.c 

history.o: history.c history.h 
	gcc -c history.c

clean:
	rm -f *o server client
