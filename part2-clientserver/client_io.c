#include "client_io.h"
#include "history.h"

void redirectstdin(const char* stdin_file) {
  if ((access(stdin_file, F_OK) == -1)) {
    return; // File does not exist
  }
  int stdin_desc;
  stdin_desc = open(stdin_file, O_RDONLY);
  dup2(stdin_desc, STDIN_FILENO);
}

void redirectstdout(const char* stdout_file, char mode) {
  int stdout_desc;
  if (mode == 'w') {
    stdout_desc = open(stdout_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  }
  if (mode == 'a') {
    stdout_desc = open(stdout_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
  } 
  dup2(stdout_desc, STDOUT_FILENO);
}

void redirectstderr(const char* stderr_file, char mode) {
  int stderr_desc;
  if (mode == 'w') {
    stderr_desc = open(stderr_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  }
  if (mode == 'a') {
    stderr_desc = open(stderr_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
  } 
  dup2(stderr_desc, STDERR_FILENO);
}

// Intialize the Buffer struct to be passed to all functions. 
void initializeLineBuffer(LineBuffer *buffer, char *line, int size, int historyCount, int historyTotal, char *prompt, FILE *historyfile){
  buffer->line = line; 
  buffer->length = 0; 
  buffer->cursor = 0; 
  buffer->size = size; 
  buffer->historyfile = historyfile; 
  buffer->historyCount = historyCount; 
  buffer->historyTotal = historyTotal; 
  buffer->prompt = prompt; 
}

// Raw mode use for inpput handling 
void setupRawMode(struct termios *oldToi){
  tcgetattr(0, oldToi); 
  struct termios raw = *oldToi;

  // Enter Raw mode 
  raw.c_lflag &= ~(ECHO | ECHOE | ICANON); // Disable Canonical mode and Echoing 
  tcsetattr(0, TCSANOW, &raw); // Set raw mode immediately 
  return;
}

// Clears current line 
void clearLine(){
  // Remove current command 
  printf("\r");
  printf("\033[K");
}

// Redraws line
void redrawLine(LineBuffer *buffer){
  int length = strlen(buffer->line);

  // Only replace with null terminator if newline is found 
  if(length > 0 && buffer->line[length - 1] == '\n'){ 
    buffer->line[strlen(buffer->line) - 1] = '\0'; // Replace \n with null
  }

  buffer->length = strlen(buffer->line); // Set cursor and length to end of line 
  buffer->cursor = buffer->length; 
  printf("%s%s", buffer->prompt, buffer->line);
  fflush(stdout);
}

// Handle Normal character Inputs 
void handleChar(LineBuffer *buffer, char ch){
  if(buffer->length < buffer->size - 1){
    // Move memory first 
    memmove(&buffer->line[buffer->cursor+1], &buffer->line[buffer->cursor], buffer->length - buffer->cursor + 1);
    buffer->line[buffer->cursor] = ch; // Enter the character in the current cursor position  
    buffer->cursor++;
    buffer->length++; 
    
    buffer->line[buffer->length] = '\0'; // Length will not change position. Always keep as null terminator 
    printf("%s", &buffer->line[buffer->cursor-1]); // Print string up until currently inserted character 
    for(int n = buffer->cursor; n < buffer->length; n++){
      printf("\033[D"); // Print cursor 
    }
    
    fflush(stdout);
  }

  return; 
}

// Handle Backspace. Deletes char from line 
void handleBackspace(LineBuffer *buffer){
  // Reduce length and memory if cursor is not at the start of string 
  if(buffer->cursor > 0){
    buffer->cursor--;  
    buffer->length--;

    // memmove is used to copy block of memory from one place to another 
    // We move the source (cursor - 1) to the destination/cursor location (cursor)
    // &line[cursor] is where the ddata will be placed 
    // &line[cursor+1] Where the data is copied from 
    // length - cursor + 1 is the length of the memory block 
    // We move the character after the current character to the position of the current character. 
    memmove(&buffer->line[buffer->cursor], &buffer->line[buffer->cursor+1], buffer->length - buffer->cursor + 1);
    buffer->line[buffer->length] = '\0';
    printf("\033[D"); // Move cursor left 
    printf("%s", &buffer->line[buffer->cursor]); // Redraw remaining line 
    printf("\033[K"); // Clear line
    
    // Reprint Cursor 
    for(int i= buffer->cursor; i < buffer->length; i++) printf("\033[D");
    fflush(stdout); 
  }

  return; 
}

// Replays History when Arrow Key Up is pressed 
void loadPreviousHistory(LineBuffer *buffer){
  if (buffer->historyCount > 1) {
    buffer->cursor = 0;
    buffer->length = 0;
    
    buffer->historyCount--; // Move up by one line in the history file
    getLineOfHistory(buffer->historyfile, buffer->historyCount, buffer->line);
    clearLine();
    redrawLine(buffer);
  } 

  return; 
}

// Loads next command in history when Arrow Key Down is pressed 
void loadNextHistory(LineBuffer *buffer){
  clearLine();

  if (buffer->historyCount < buffer->historyTotal) {
    buffer->historyCount++; // Move down by one line in the history file
    getLineOfHistory(buffer->historyfile, buffer->historyCount, buffer->line);
    clearLine();
    redrawLine(buffer);
  } 
  
  // Bottom of history resets to empty line 
  else{ 
    buffer->historyCount = buffer->historyTotal + 1; 
    buffer->line[0]= '\0';
    buffer->length = 0; 
    buffer->cursor = 0; 
    printf("%s", buffer->prompt); 
    fflush(stdout);
  }
  
  return; 
}

// Arrow Key controller which redirects depending on the arrow key press 
void handleArrowKey(LineBuffer *buffer){
  char seq[2]; // Completed Escape sequence for arrow keys 
  
  // Read the next 2 bytes and only execute if the next 2 bytes are successfully read 
  if(read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1){
    
    // Switch Case statement for arrow keys 
    if(seq[0] == '['){ 
      
      switch(seq[1]){
        // Load previous history in history file (Arrow Key Up) 
        case 'A': 
          loadPreviousHistory(buffer);
          break; 
        
        // Load Next history in history file (Arrow Key Down)
        case 'B':
          loadNextHistory(buffer); 
          break; 

        // Move Cursor right 
        case 'C':
          if(buffer->cursor < buffer->length){
            buffer->cursor++; 
            printf("\033[C");
            fflush(stdout); 
          }
          break; 

        // Move Cursor left
        case 'D':
          if(buffer->cursor > 0){
            buffer->cursor--; 
            printf("\033[D");   
            fflush(stdout); 
          }
          break; 
        default:
          break; 
      }
    }
  }
  return;  
}

// General Input line function to read each user input without pressing enter 
// Required for arrow key functionality 
int readLine(char *line, int size, char *prompt, FILE* historyfile){ 
  int bytes; // Num of bytes read 
  struct termios oldToi; // Canonical terminal mode 
  
  // Initialize Buffer struct 
  LineBuffer buffer; 
  line[0] = '\0';
  char lineOfHistory[COMMAND_LINE_SIZE]; // Buffer for storing a line from the history file
  int numberOfLinesOfHistory = getLineOfHistory(historyfile, -1, lineOfHistory); // Total History 
  int lineOfHistoryNumber = numberOfLinesOfHistory + 1; // History Count. Decrements/Increments when arrow key pressed 
  initializeLineBuffer(&buffer, line, size, lineOfHistoryNumber, numberOfLinesOfHistory, prompt, historyfile);

  char ch; // User entered character 
  setupRawMode(&oldToi); 

  printf("%s", prompt); // Print prompt first before characters 
  fflush(stdout); // Flush output buffer 
  
  // Input will keep looping until newline is entered or error in read occurs 
  // Prints each character to the terminal 
  while((bytes = read(0, &ch, 1)) == 1){
    // For Normal Characters 
    if(ch >= 32 && ch <= 126){
      handleChar(&buffer, ch); 
    }
    
    // For Backspace
    else if(ch == 127){
      handleBackspace(&buffer);
    }
    
    // For Enter 
    // TODO: CHANGE THIS TO BREAK, MAKE A RETURNTOTERMINAL FUNCTION 
    else if(ch == '\n'){
      buffer.line[buffer.length] = '\0'; 
      printf("\n"); 
      tcsetattr(0, TCSANOW, &oldToi); // Revert back to canonical mode 
      return buffer.length; 
    }

    // For arrow keys 
    if(ch == '\033'){
      handleArrowKey(&buffer);   
    }
  }

  tcsetattr(0, TCSANOW, &oldToi); // Revert back to canonical mode 
  if(bytes == 0 ){
    return -1;
  }

  return buffer.length; 
}
