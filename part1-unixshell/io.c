#include "io.h"

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

// General Input line function to read each user input without pressing enter 
// Required for arrow key functionality 
int readLine(char *line, int size, char *prompt){ 
  int bytes; // Num of bytes read 
  line[0] = '\0';
  char ch; // User entered character 
  int length = 0; // Number of characters in line 
  int cursor = 0; // Cursor position 
  struct termios oldToi; // Canonical terminal mode 
  tcgetattr(0, &oldToi); 
  struct termios raw = oldToi;

  // Enter Raw mode 
  raw.c_lflag &= ~(ECHO | ECHOE | ICANON); // Disable Canonical mode and Echoing 
  tcsetattr(0, TCSANOW, &raw); // Set raw mode immediately 
  
  printf("%s", prompt); // Print prompt first before characters 
  fflush(stdout); // Flush output buffer 
  
  // Input will keep looping until newline is entered or error in read occurs 
  // Prints each character to the terminal 
  while((bytes = read(0, &ch, 1)) == 1){
    // For Normal Characters 
    if(ch >= 32 && ch <= 126){
      if(length < size - 1){ // TODO: Maybe change to -1. -2 is for newline and null 
        // Move memory first 
        memmove(&line[cursor+1], &line[cursor], length - cursor + 1);
        line[cursor] = ch; // Enter the character in the current cursor position  
        cursor++;
        length++; 
        
        line[length] = '\0'; // Length will not change position. Always keep as null terminator 
        printf("%s", &line[cursor-1]); // Print string up until currently inserted character 
        for(int n = cursor; n < length; n++){
          printf("\033[D"); // Print cursor 
        }
        
        fflush(stdout);
      }
    }
    
    // For Backspace
    else if(ch == 127){
      // Reduce length and memory if cursor is not at the start of string 
      if(cursor > 0){
        cursor--;  
        length--; 

        // memmove is used to copy block of memory from one place to another 
        // We move the source (cursor - 1) to the destination/cursor location (cursor)
        // &line[cursor] is where the ddata will be placed 
        // &line[cursor+1] Where the data is copied from 
        // length - cursor + 1 is the length of the memory block 
        // We move the character after the current character to the position of the current character. 
        memmove(&line[cursor], &line[cursor+1], length - cursor + 1);
        line[length] = '\0';
        printf("\033[D"); // Move cursor left 
        printf("%s", &line[cursor]); // Redraw remaining line 
        printf("\033[K"); // Clear line
        
        // Reprint Cursor 
        for(int i=cursor;i<length;i++) printf("\033[D");
        fflush(stdout); 
      }
    }
    
    // For Enter 
    else if(ch == '\n'){
      line[length] = '\0'; 
      printf("\n"); 
      tcsetattr(0, TCSANOW, &oldToi); // Revert back to canonical mode 
      return length; 
    }

    // For arrow keys 
    if(ch == '\033'){
      char seq[2]; // Completed Escape sequence for arrow keys 
      
      // Read the next 2 bytes and only execute if the next 2 bytes are successfully read 
      if(read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1){
        
        // Switch Case statement for arrow keys 
        if(seq[0] == '['){ 
          switch(seq[1]){
            // Replay History 
            case 'A': 
              // TODO: Add a function to get the history by 1 line before but dont execute 
              break; 
            case 'B':
              // TODO: Add a function to get the the next history. Exits with one line.
              break; 
            case 'C':
              if(cursor < length){
                cursor++; 
                printf("\033[C");
                fflush(stdout); 
              }
              break; 
            case 'D':
              if(cursor > 0){
                cursor--; 
                printf("\033[D");   
                fflush(stdout); 
              }
              break; 
            default:
              break; 
          }
        }
      }
    } 
  }

  tcsetattr(0, TCSANOW, &oldToi); // Revert back to canonical mode 
  if(bytes == 0 ){
    return -1;
  }

  return length; 
}
