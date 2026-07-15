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

void history();

void clear(FILE *historyfile);

void change_prompt(char* prompt, char* newPrompt);

void reenact_history(int lineNumberToReenact, char* inputLine, int* reenactingHistory);

void ignore_interrupts();

void getLineOfHistory(int lineNumberToGet, char* lineToReturnTo);
