/**
* File: main.c 
* Description: Read one line at a time and execute commands
* Date: 6/5/2026
* TODO: Dont take exit into history file 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glob.h>
#include <fcntl.h>
#include "token.h"
#include "command.h"
#include "builtin.h"

void saveHistory(char *inputLine, FILE *historyfile);
int executeBuiltIn(Command *cmd, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory);
void executeCommand(Command *command); // Forks external commands (Seperators ; and &)
void runProgram(Command *command); // Runs the program 
int executePipe(Command commands[], int first, int last); 
void claim_children(int sig); // Function to collect zombie processes
void signalHandlerSetup();
int expandWildCard(char *token[], char *expandedTokens[], char expandedStorage[][COMMAND_LINE_SIZE], int tokenSize);
void redirectstdin (char* stdin_file);
void redirectstdout(char* stdout_file);
void redirectstderr(char* stderr_file);

int main(int argc, char *argv[]){
  // Initialize Variables
  char inputLine[COMMAND_LINE_SIZE];
  char *token[MAX_NUM_TOKENS]; // Inital tokenized lines 
  char expandedStorage[MAX_NUM_TOKENS][COMMAND_LINE_SIZE];
  char *expandedTokens[MAX_NUM_TOKENS]; // After glob() for wildcard implementation
  Command command[MAX_NUM_COMMANDS];
  char prompt[256] = "$ ";
  int saved_stdin  = dup(0); // For reverting to after redirecting
  int saved_stdout = dup(1); // For reverting to after redirecting
  int saved_stderr = dup(2); // For reverting to after redirecting

  signalHandlerSetup(); // Register signal handler 
  ignore_interrupts(); // Disables CTRL+C / CTRL+Z / CTRL+\ 
 
  // History initialization 
  char historypath[4096];
  getcwd(historypath, sizeof(historypath));
  if (historypath == NULL) {
    perror("getcwd failed");
    return -1;
  }
  strcat(historypath, "/");
  strcat(historypath, HISTORY_FILE);
  FILE *historyfile;
  historyfile = fopen(historypath, "a+");
  int reenactingHistory = 0;

  while(1){
    // Reset the descriptors of stdin and stdout if they were redirected
    dup2(saved_stdin , 0);
    dup2(saved_stdout, 1);
    dup2(saved_stderr, 2);

    printf("%s", prompt);
     
    if (reenactingHistory == 0) {
      // Breaks out the loop if fgets fails. 
      if(fgets(inputLine, COMMAND_LINE_SIZE, stdin) == NULL) {
        fclose(historyfile);
        break;
      }      
    } else if (reenactingHistory == 1) {
      reenactingHistory = 0; // allows the user to enter input in the next iteration
    }

    inputLine[strcspn(inputLine, "\n")] = '\0'; // Pattern for removing saved newline
    
    // Skip empty input lines
    if(strlen(inputLine) == 0){
      continue;
    }
    
    if(strncmp(inputLine, "!", 1) != 0 && strlen(inputLine) > 0 && strncmp(inputLine, "exit", 4) != 0 && strncmp(inputLine, "history", 7) != 0){
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
      printf("Stdin file: %s\n", command[n].stdin_file);
      printf("Stdout file: %s\n", command[n].stdout_file);

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

  if (command->stdin_file != NULL) {
    redirectstdin(command->stdin_file);
  }

  if (command->stdout_file != NULL) {
    redirectstdout(command->stdout_file);
  }

  // Exits the program after exit is input. 
  if (strcmp(cmd, "exit") == 0) {
    //dup2(saved_stdin , 0);
    //dup2(saved_stdout, 1);
    printf("Exiting shell...\n");
    fclose(historyfile);
    exit(0); // bug: crashes if stdin and stdout are not set to their default values
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
    //history(historypath);
    history(historyfile);
    return 1; 
  } 

  // Executes line of code from history with corresponding history index 
  if (strncmp(cmd, "!", 1) == 0) {
    // Executing previous command 
    if (strncmp(cmd, "!!", 2) == 0) {
      getLineOfHistory(historyfile, -1, inputLine);
      *reenactingHistory = 1;
    } else { // Extract the number after the !
      int strnum = 0;
      for (int i = 1; i < strlen(cmd) && isdigit(cmd[i]); i++) { // Iterate over the characters after !
        if (isdigit(cmd[i])) { // If the character is a digit
          char digit[2] = {cmd[i], '\0'}; // Create a C string with just that digit
          strnum = strnum * 10 + atoi(digit); // Convert the C string to a number and add it to strnum
        }
      }
      getLineOfHistory(historyfile, strnum, inputLine);
      *reenactingHistory = 1;
    }
    return 2;
  }

  return 0; // If command is not a Built in shell comamnd 
}

void redirectstdin(char* stdin_file) {
  if ((access(stdin_file, F_OK) == -1)) {
    return; // File does not exist
  }
  int stdin_desc = open(stdin_file, O_RDONLY);
  dup2(stdin_desc, STDIN_FILENO);
}

void redirectstdout(char* stdout_file) {
  if ((access(stdout_file, F_OK) == -1)) {
    FILE* newFile = fopen(stdout_file, "w");
    fclose(newFile); // Open and close to create a new file if it does not exist
  }
  int stdout_desc = open(stdout_file, O_WRONLY | O_APPEND);
  dup2(stdout_desc, STDOUT_FILENO);
}

// TODO: How and where can we use this function?
void redirectstderr(char* stderr_file) {
  if ((access(stderr_file, F_OK) == -1)) {
    FILE* newFile = fopen(stderr_file, "w");
    fclose(newFile); // Open and close to create a new file if it does not exist
  }
  int stderr_desc = open(stderr_file, O_WRONLY | O_APPEND);
  dup2(stderr_desc, STDERR_FILENO);
}

// Executes external Non-Built in Unix commands usign execp 
void executeCommand(Command *command){
  // Input redirection and output redirection 
  if (command->stdin_file != NULL) {
    redirectstdin(command->stdin_file);
  }

  if (command->stdout_file != NULL) {
    redirectstdout(command->stdout_file);
  }

  int pid = fork(); // All external commands will be handled by child processes 
  
  if(pid == 0){ // In Child 
    runProgram(command);
  }

  if(pid < 0){ // Fork failure 
    perror("fork");
    return;
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

// Conducts the process of running the program inside of child fork 
void runProgram(Command *command){
  // Input Redirection
  if (command->stdin_file != NULL) {
    redirectstdin(command->stdin_file);
  }

  // Output Redirection
  if (command->stdout_file != NULL) {
    redirectstdout(command->stdout_file);
  }

  execvp(command->argv[0], command->argv); // Run command in child process 
  perror("execv failed");
  exit(1); 
} 

// Execute Pipe Unix commands 
int executePipe(Command commands[], int first, int last){
  int p[2]; // 0 for read | 1 for write
  pid_t pid; 
  int input = STDIN_FILENO; // stores read end of previous pipe 

  // Fork each command into a seperate child process 
  for(int n = first; n <= last; n++){
    
    // Create pipes for each command 
    if(n != last){
      if(pipe(p) < 0){
        perror("Pipe Call"); return -1; 
      }
    }

    pid = fork();

    if(pid < 0){
      perror("fork"); 
      return -1;
    }

    // Process for each pipes 
    if(pid == 0){
      // Connect previous pipe to input 
      if(input != STDIN_FILENO){
        dup2(input, STDIN_FILENO);
        close(input);
      }

      // Connect stdout to the next pipe 
      if(n != last){
        dup2(p[1], STDOUT_FILENO);
        close(p[0]);
        close(p[1]);
      }

      runProgram(&commands[n]);
    }

    // In Parent, Close pipe ends 
    if(pid > 0){
      if(input != STDIN_FILENO){
        close(input);
      }

      // Save Input 
      if(n != last){
        close(p[1]); // Close write end
        input = p[0]; // Save input 
      }
    }
  }

  // Wait for the pipeline commands to finish 
  for(int n = first; n <= last; n++){
    wait(NULL);
  }

  return 0;
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
  act.sa_flags = SA_NOCLDSTOP | SA_RESTART; // Not catch sopped children 
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
