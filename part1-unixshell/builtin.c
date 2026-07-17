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

void cd(const char * path) {
  if (path == NULL) {
      return;
  } else if (chdir(path) != 0) {
    perror("cd");
  }
  return;
}

void walk(const char * path) {
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

// BUG: Prompts with spaces are not processed properly
void change_prompt(char* prompt, const char* newPrompt) {
  strcpy(prompt, newPrompt);
  return;
}


