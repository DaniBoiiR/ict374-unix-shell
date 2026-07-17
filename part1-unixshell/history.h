#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define HISTORY_FILE ".shell_history"
#define COMMAND_LINE_SIZE 100 // Global variable (BAD) for command line size 

void history(FILE *historyfile);

FILE *initializeHistory();

void saveHistory(char *inputLine, FILE *historyfile);

void getLineOfHistory(FILE* historyfile, int lineNumberToGet, char* lineToReturnTo);

void getLineOfHistoryByString(FILE* historyfile, const char* substringToSearchFor, char* lineToReturnTo);
