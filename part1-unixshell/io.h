#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

void redirectstdin (const char* stdin_file);

void redirectstdout(const char* stdout_file, char mode);

void redirectstderr(const char* stderr_file, char mode);

int readLine(char *line, int size, char *prompt, FILE* historyfile);
