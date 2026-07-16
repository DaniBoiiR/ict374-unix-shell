#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#define COMMAND_LINE_SIZE 100 // Global variable (BAD) for command line size 
#define HISTORY_FILE ".shell_history"

void pwd();

void cd(char * path);

void walk(char * path);

void history(FILE *historyfile);

void change_prompt(char* prompt, char* newPrompt);

void ignore_interrupts();

void getLineOfHistory(FILE* historyfile, int lineNumberToGet, char* lineToReturnTo);
