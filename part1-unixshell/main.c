/**
* File: main.c 
* Description: Read one line at a time and execute commands
* Date: 6/5/20206
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "token.h"
#include "command.h"
#define COMMAND_LINE_SIZE 100 // Global variable (BAD) for command line size 

int main(int argc, char *argv[]){
  // Initialize Variables
  char inputLine[COMMAND_LINE_SIZE];
  char *token[MAX_NUM_TOKENS];
  Command command[MAX_NUM_COMMANDS];

  while(1){
    printf("$ ");

    // Breaks out the loop if fgets fails. 
    if(fgets(inputLine, COMMAND_LINE_SIZE, stdin) == NULL) 
      break;

    inputLine[strcspn(inputLine, "\n")] = '\0'; // Pattern for removing saved newline

    // Exits when 'bye' is entered 
    if(strcasecmp(inputLine, "exit") == 0){
      break;
    }

    int tokenSize = tokenize(inputLine, token);
    
    // Checks for error 
    if(tokenSize == -1){
      printf("Too many tokens\n");
      continue; 
    }
    
    // Initalize command array 
    for(int n = 0; n < MAX_NUM_COMMANDS; n++){
      initializeCommandStructure(command);
    }

    int commandSize = separateCommands(token, command);

    // Print and execute the commands 
    for(int n = 0; n < commandSize; n++){
      printf("Command %d: ", n+1);

      for(int i = command[n].first; i <= command[n].last; i++){
        printf("%s ", token[i]);
      }

      printf("\n");
      printf("Separator: %s\n", command[n].sep);
      
      // Replaces running program, does not come back and loop 
      // Future improvement: Make child process do this 
      execvp(command[n].argv[0], command[n].argv);
    }

  }
  return 0;
}

