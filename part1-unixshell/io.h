#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

void redirectstdin (char* stdin_file);

void redirectstdout(char* stdout_file);

void redirectstderr(char* stderr_file);

int readLine(char *line, int size, char *prompt);
