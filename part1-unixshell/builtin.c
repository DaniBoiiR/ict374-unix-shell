#include "builtin.h"

void pwd() {
  char cwd[4096];
  getcwd(cwd, sizeof(cwd));
  if (cwd == NULL) {
    perror("getcwd failed");
    return;
  }
  printf("%s\n", cwd);
}

void cd(char * path) {
  if (path == NULL) {
      return;
  } else if (chdir(path) != 0) {
    perror("cd");
  }
  return;
}

void walk(char * path) {
  if (path == NULL) {
    if (chdir(getenv("HOME")) != 0) {
        perror("cd");
    }
    return;
  } else if (chdir(path) != 0) {
    perror("cd");
  }
  return;
}

void history(FILE *historyfile) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
  // Move the cursor to the beginning of the file in case it is at the end
  fseek(historyfile, 0, SEEK_SET);
  while (fgets(line, sizeof(line), historyfile)) {
      printf("%d  %s", lineNumber, line);
      lineNumber++;
  }
  // Move the cursor to the beginning of the file after looping over it
  fseek(historyfile, 0, SEEK_SET);
}

// Bug: Prompts with spaces are not processed properly
void change_prompt(char* prompt, char* newPrompt) {
  strcpy(prompt, newPrompt);
  return;
}

void ignore_interrupts() {
  // Ignore Ctrl+C, Ctrl+Z and Ctrl+\

  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGQUIT);
  sigaddset(&sigs, SIGTSTP);
  sigprocmask(SIG_BLOCK, &sigs, NULL);
}

// TODO: Search history by !string 
void getLineOfHistory(FILE* historyfile, int lineNumberToGet, char* lineToReturnTo) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
    
  fseek(historyfile, 0, SEEK_SET);
  while (fgets(line, sizeof(line), historyfile)) {
    if (lineNumber == lineNumberToGet) {
        strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
        fseek(historyfile, 0, SEEK_SET);
        return;
    }
    lineNumber++;
  }
  fseek(historyfile, 0, SEEK_SET);

  if (lineNumberToGet > lineNumber) {
    strcpy(lineToReturnTo, ""); // Copy an empty string to the input line, which will be ignored in the next iteration
    fseek(historyfile, 0, SEEK_SET);
    return;
  }
  if (lineNumberToGet == -1) {
    // If it reaches here, the cursor will already be pointing at the last line
    strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
    fseek(historyfile, 0, SEEK_SET);
  }
}
