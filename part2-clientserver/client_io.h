#ifndef CLIENT_IO_H
#define CLIENT_IO_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

typedef struct {
  char *line; // The Line entered by the user, new characters are added to line and displayed ot user 
  int length; // The total length of the entered line. Updates with character insertion/deletion  
  int cursor; // Position of the cursor in the line.
  int size; // Command line size 

  FILE *historyfile; // File storing history of previously executed commands 
  int historyCount; // Current index of history in history file 
  int historyTotal; // Total number of history file lines   
  
  char *prompt; // Line prompt 

} LineBuffer; 

void redirectstdin (const char* stdin_file);

void redirectstdout(const char* stdout_file, char mode);

void redirectstderr(const char* stderr_file, char mode);

int readLine(char *line, int size, char *prompt, FILE* historyfile);

#endif
