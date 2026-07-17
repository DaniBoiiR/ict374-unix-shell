#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "command.h"
#include "io.h"
#include "builtin.h"
#include "history.h"

void executeCommand(Command *command); // Forks external commands (Seperators ; and &)

void runProgram(Command *command); // Runs the program 

int executePipe(Command commands[], int first, int last); 

int executeBuiltIn(Command *cmd, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory);
