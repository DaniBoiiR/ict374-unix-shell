/**
* File: main.c 
* Description: Read one line at a time and execute commands
* Date: 6/5/2026
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "token.h"
#include "command.h"
#include "builtin.h"

void saveHistory(char *inputLine, FILE *historyfile);
int executeBuiltIn(Command *cmd, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory);

int main(int argc, char *argv[]){
  // Initialize Variables
  char inputLine[COMMAND_LINE_SIZE];
  char *token[MAX_NUM_TOKENS];
  Command command[MAX_NUM_COMMANDS];
  char prompt[256] = "$ ";

  ignore_interrupts();
 
  // History initialization 
  FILE *historyfile;
  historyfile = fopen(HISTORY_FILE, "a");
  int reenactingHistory = 0;

  while(1){
    printf("%s", prompt);
     
    if (reenactingHistory == 0) {
      // Breaks out the loop if fgets fails. 
      if(fgets(inputLine, COMMAND_LINE_SIZE, stdin) == NULL) {
        fclose(historyfile);
        break;
      }
    } else if (reenactingHistory == 1) {
      // reenact_history() sets the input line to the chosen line of history
      reenactingHistory = 0; // allows the user to enter input in the next iteration
    }

    inputLine[strcspn(inputLine, "\n")] = '\0'; // Pattern for removing saved newline
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

      // Exits the program after exit is input. 
      if (strcmp(command[n].argv[0], "exit") == 0) {
        printf("Exiting shell...\n");
        fclose(historyfile);
        exit(0);
      } 

      // Save input line to history
      fputs(inputLine, historyfile);
      fputs("\n", historyfile);
      int flush = fflush(historyfile);
      printf("%d", flush);

      // Execute Built in shell commands. 
      if(executeBuiltIn(&command[n], prompt, historyfile, inputLine, &reenactingHistory){
        continue; 
      }

      // Execute Unix Shell Commands 
      else{

        int pid = fork();
        if(pid > 0){ // In Parent 
          waitpid(pid, NULL, 0);
        }
        if (/*this is the child (the command)*/ pid == 0) {
          execvp(command[n].argv[0], command[n].argv);
          perror("execv failed");
        }
        if (/*fork failed*/ pid < 0) {
          perror("fork");
        }
      }
    }
  }
  return 0;
}

// Executes Built in Commands 
int executeBuiltIn(Command *cmd, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory){
  char *cmd = command->argv[0]; 

  // Return 1 If commands are Built in 
  if (strcmp(cmd, "pwd") == 0) {
    pwd();
    return 1;
  }

  if (strcmp(cmd, "cd") == 0) {
    cd(command->argv[1]);    
    return 1;
  } 

  if (strcmp(cmd, "walk") == 0) {
    walk(command->argv[1]);
    return 1; 
  } 

  if (strcmp(cmd, "prompt") == 0) {
    change_prompt(prompt, command->argv[1]);
    return 1; 
  }

  if (strcmp(cmd, "history") == 0) {
    history();
    return 1; 
  } 

  if (strcmp(cmd, "clear") == 0) {
    clear(historyfile);
    return 1; 
  } 

  if (strcmp(cmd, "!!") == 0) {
    reenact_history(-1, inputLine, &reenactingHistory);
    return 1;
  }

  return 0; // If command is not a Built in shell comamnd 
}
