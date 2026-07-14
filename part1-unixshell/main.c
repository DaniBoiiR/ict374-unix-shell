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
#include <glob.h>
#include "token.h"
#include "command.h"
#include "builtin.h"

void saveHistory(char *inputLine, FILE *historyfile);
int executeBuiltIn(Command *cmd, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory);
void executeCommand(Command *command); // Executes external commands (Seperators ; and &)
//int executePipe(Command *command); // Executes Pipes (|)
void claim_children(int sig); // Function to collect zombie processes
void signalHandlerSetup();
int expandWildCard(char *token[], char *expandedTokens[], char expandedStorage[][COMMAND_LINE_SIZE], int tokenSize);
  
int main(int argc, char *argv[]){
  // Initialize Variables
  char inputLine[COMMAND_LINE_SIZE];
  char *token[MAX_NUM_TOKENS]; // Inital tokenized lines 
  char expandedStorage[MAX_NUM_TOKENS][COMMAND_LINE_SIZE];
  char *expandedTokens[MAX_NUM_TOKENS]; // After glob() for wildcard implementation
  Command command[MAX_NUM_COMMANDS];
  char prompt[256] = "$ ";


  signalHandlerSetup(); // Register signal handler 
  ignore_interrupts(); // Disables CTRL+C / CTRL+Z / CTRL+\ 
 
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
    
    // Skip empty input lines
    if(strlen(inputLine) == 0){
    continue;
}
    
    if(strcmp(inputLine, "!!") != 0 && strlen(inputLine) > 0){
      saveHistory(inputLine, historyfile);
    }

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
    for(int n = 0; n < commandSize; n++){

      printf("Command %d: ", n+1);

      for(int i = command[n].first; i <= command[n].last; i++){
        printf("%s ", expandedTokens[i]);
      }
      printf("\n");
      printf("Separator: %s\n", command[n].sep);

      // Execute Built in shell commands. 
      int result = executeBuiltIn(&command[n], prompt, historyfile, inputLine, &reenactingHistory);

      if(result == 1){
        continue; 
      }

      // IF !! is entered. break loop 
      if(result == 2){
        break;
      }

      // Execute Unix Shell Commands 
      else{
        executeCommand(&command[n]);
      }
    }
  }
  return 0;
}

// Save History
// TODO: ADD to the built in commands instead of putting this on main 
void saveHistory(char *inputLine, FILE *historyfile){
  // First checks if fputs failed 
  if(fputs(inputLine, historyfile) == EOF ||fputs("\n", historyfile) == EOF){
    perror("Failed to save history");
    return; 
  }
  
  int flush = fflush(historyfile);
  //printf("%d", flush); // Uncomment for debugging
}

// Executes Built in Commands 
int executeBuiltIn(Command *command, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory){
  // Null protection 
  if(command->argv == NULL || command->argv[0] == NULL) return 0;
  char *cmd = command->argv[0]; 

  // Exits the program after exit is input. 
  if (strcmp(cmd, "exit") == 0) {
    printf("Exiting shell...\n");
    fclose(historyfile);
    exit(0);
  }

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
    reenact_history(-1, inputLine, reenactingHistory);
    return 2;
  }

  return 0; // If command is not a Built in shell comamnd 
}

// Executes external Non-Built in Unix commands usign execp 
void executeCommand(Command *command){
  int pid = fork(); // All external commands will be handled by child processes 

  if(pid < 0){ // Fork failure 
    perror("fork");
    return;
  }

  if(pid == 0){ // In Child 
    execvp(command->argv[0], command->argv); // Run command in child process 
    perror("execv failed");
    exit(1);
  }

  // In Parent
  // Parent Handling Background processes and & operator
  else{
    if(strcmp(command->sep, "&") != 0){
      // Sequential Handling. Shell has to wait until child process is complete
      // After child is complete, shell returns to main loop 
      waitpid(pid, NULL, 0);
    }
  }
}

// Claims zombie child processes when child process is finished executing 
void claim_children(int sig){
  pid_t pid = 1; 
  
  // Claim zombied by collecting exit status through waitpid (retrives exit status of specific pid)
  // -1 = Wait for any child process in process group 
  // NULL ignores child exit status
  // WNOHANG = Prevents functions blocking. 
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Sets up signal handler to collect zombie child processes
void signalHandlerSetup(){
  struct sigaction act; 
  act.sa_handler = claim_children; // Assign function pointer for reliable signal
  sigemptyset(&act.sa_mask); // Dont block other signals 
  act.sa_flags = SA_NOCLDSTOP; // Not catch sopped children 
  sigaction(SIGCHLD, &act, NULL); // When a zombie signal is found, claim children is fired by the signal handler 
}

// TODO: Move this to token.c and token.h respectively 
// BUG: potential overflow with over 500 files if found. add edge case check 
int expandWildCard(char *token[], char *expandedTokens[], char expandedStorage[][COMMAND_LINE_SIZE], int tokenSize){
  int count = 0; 

  // Exit if empty token 
  if(tokenSize == 0) return 0;

  for(int n = 0; n < tokenSize; n++){
    
    // If wild card is found 
    if((strchr(token[n], '*')) != NULL || strchr(token[n], '?') != NULL){
      
      // results store Count of matched paths, List of matched pathnames, and Slots reserve in gl_pathv
      glob_t results; 
      int status = glob(token[n], 0, NULL, &results); // Glob searches matching patterns 

      // If glob returns successful/matches found, start token expansion 
      if(status == 0){
        for(int i = 0; i < results.gl_pathc; i++){
            
          // Handle overflow 
          if(count >= MAX_NUM_TOKENS -1){
            globfree(&results);
            return -1;
          }

          // Copy into expanded tokens array 
          strcpy(expandedStorage[count], results.gl_pathv[i]);
          expandedTokens[count] = expandedStorage[count]; // gl_pathv stores file names 
          count++; 
        }
      } 
       
      // If no matches found, keep original token
      else {
        strcpy(expandedStorage[count], token[n]);
        expandedTokens[count] = expandedStorage[count];
        count++; 
      }
      
      globfree(&results); // Frees dynamically allocated memory 
    } 

    // Normal Tokens 
    else{
      if(count >= MAX_NUM_TOKENS -1){
        return -1;
      }
      strcpy(expandedStorage[count], token[n]);
      expandedTokens[count] = expandedStorage[count];
      count++; 
    }
  }

  expandedTokens[count] = NULL; // null terminator 
  return count; 
}
