/**
* File: main.c 
* Description: Read one line at a time and execute commands
* Date: 6/5/2026
*/

#include <stdio.h>
#include <errno.h>
#include "token.h"
//#include "command.h"
#include "builtin.h"
#include "history.h"
#include "signal.h"
#include "io.h"
#include "execute.h"

void printCommand(Command* command, char** expandedTokens, int n);

int main(int argc, char *argv[]){
  // Initialize Variables
  char inputLine[COMMAND_LINE_SIZE];
  char *token[MAX_NUM_TOKENS]; // Inital tokenized lines 
  char expandedStorage[MAX_NUM_TOKENS][COMMAND_LINE_SIZE];
  char *expandedTokens[MAX_NUM_TOKENS]; // After glob() for wildcard implementation
  Command command[MAX_NUM_COMMANDS];
  char prompt[256] = "% ";
  int debugMode = 0; 
  int again = 1; // For slow system call handling
  //TODO: Replace these with a struct
  int saved_stdin  = dup(0); // For reverting to after redirecting
  int saved_stdout = dup(1); // For reverting to after redirecting
  int saved_stderr = dup(2); // For reverting to after redirecting

  signalHandlerSetup(); // Register signal handler 
  ignore_interrupts(); // Disables CTRL+C / CTRL+Z / CTRL+\ 

  // History initialization 
  FILE *historyfile;
  historyfile = initializeHistory();
  int reenactingHistory = 0;

  while(1){
    // Reset the descriptors of stdin and stdout if they were redirected
    dup2(saved_stdin , 0);
    dup2(saved_stdout, 1);
    dup2(saved_stderr, 2);
    
    again = 1;
    if (reenactingHistory == 0) {
      // Breaks out the loop if readLine fails.
      while (again) {
        again = 0;
        
        // Local Interactive terminal 
        if(isatty(STDIN_FILENO)){
          if(readLine(inputLine, COMMAND_LINE_SIZE, prompt, historyfile) == -1) {
            fclose(historyfile);
            break;
          }
          if (inputLine == NULL) {
            if (errno = EINTR) {
              again = 1;
            }
          }
        } else {
          printf("%s", prompt); 
          fflush(stdout); 

          if(fgets(inputLine, COMMAND_LINE_SIZE, stdin) == NULL) {
            fclose(historyfile);
            break; 
          }
        }
      }
        
    } else if (reenactingHistory == 1) {
      reenactingHistory = 0; // allows the user to enter input in the next iteration
    }

    // Don't remove this because otherwise, using ! to repeat history leaves unwanted new lines
    inputLine[strcspn(inputLine, "\n")] = '\0'; // Pattern for removing saved newline
    
    // Skip empty input lines
    if(strlen(inputLine) == 0){
      continue;
    }
    
    saveHistory(inputLine, historyfile);

    int tokenSize = tokenize(inputLine, token);
    int expandedTokensSize = expandWildCard(token, expandedTokens, expandedStorage, tokenSize);
    
    // Checks for error 
    if(expandedTokensSize == -1){
      printf("Too many tokens\n");
      continue; 
    }
    
    // Initalize command array 
    for(int n = 0; n < MAX_NUM_COMMANDS; n++){
      initializeCommandStructure(&command[n]);
    }

    int commandSize = separateCommands(expandedTokens, command);

    // Skip Empty Command 
    if(commandSize == 0) continue; 

    // Print and execute the commands 
    for(int n = 0; n < commandSize; n++){\
      
      // Only print debug information when debug mode is on 
      if(debugMode){
        printCommand(command, expandedTokens, n); // Comment this out when not debugging
      }

      // Creating a pipeline if Pipes are present in the command
      if(strcmp(command[n].sep, "|") == 0){
        int first = n; // Current index. Start of pipeline 
        int last = n; // End of pipeline 

        // Find end of pipeline 
        while(last < commandSize - 1 && strcmp(command[last].sep, "|") == 0){
          last++;
        }

        executePipe(command, first, last);
        n = last;
        continue; 
      }

      // Execute Built in shell commands. 
      int result = executeBuiltIn(&command[n], prompt, historyfile, inputLine, &reenactingHistory);
      
      // For built in commands
      if(result == 1){
        continue; 
      }

      // IF ! is entered. break loop 
      if(result == 2){
        break;
      }

      // Checks if debug mode is entered 
      if(result == 3){
        debugMode = 1; 
        continue; 
      }

      if(result == 4){
        debugMode = 0; 
        continue; 
      }

      // Execute Unix Shell Commands 
      else{
        executeCommand(&command[n]);
      }
    }
  }
  close(saved_stdin);
  close(saved_stdin);
  close(saved_stderr);

  return 0;
}

void printCommand(Command* command, char** expandedTokens, int n) {
  printf("Command %d: ", n+1);

  for(int i = command[n].first; i <= command[n].last; i++){
    printf("%s ", expandedTokens[i]);
  }

  printf("\n");
  printf("Separator: %s\n", command[n].sep);
  printf("Stdin file: %s\n", command[n].stdin_file);
  printf("Stdout file: %s\n", command[n].stdout_file);
  printf("Stderr file: %s\n", command[n].stderr_file);
}
