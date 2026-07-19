# Makefile for Unix shell 

shell: main.o token.o command.o builtin.o history.o signal.o io.o execute.o
	gcc main.o token.o command.o builtin.o history.o signal.o io.o execute.o -o shell 

main.o: main.c token.h command.h history.h signal.h io.h execute.h
	gcc -c main.c 

token.o: token.c token.h 
	gcc -c token.c 

command.o: command.c command.h 
	gcc -c command.c

builtin.o: builtin.c builtin.h
	gcc -c builtin.c

history.o: history.c history.h
	gcc -c history.c

signal.o: signal.c signal.h
	gcc -c signal.c

io.o: io.c io.h
	gcc -c io.c

execute.o: execute.c execute.h
	gcc -c execute.c

clean: 
	rm *.o 
	rm .shell_history
