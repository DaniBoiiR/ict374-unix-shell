/** 
* File: server.c 
* Description: Mintains connection with client allowing client/server message exchanges, echoing users inputs. Exits when "quit" is sent 
* Steps: Assign Socket to Server -> Bind an IP address -> Listen for connection -> Accept Connection -> Receive Message -> Send Echoed Message 
* Date: 7/7/2026
*/ 

#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h> // for gethostbyname()
#include <string.h>
#include <stdio.h>
#include <signal.h> 
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "stream.h"

#define SERVER_TCP_PORT 4005 // Server port number where the server listens to incoming client connections

struct account {
    char* username;
    char* password;
};

typedef struct account Account;

void initialize_account(Account* account, char* username, char* password) {
  account->username = username;
  account->password = password;
}

// Claims zombie child processes when child process is finished executing 
void claim_children(int sig){
  pid_t pid = 1; 
  
  // Claim zombied by collecting exit status through waitpid (retrives exit status of specific pid)
  // 0 = Wait for any child process | (int *)0 ignores child exit status | WNOHANG = Prevents functions blocking. 
  while(pid > 0){
    pid = waitpid(0, (int *)0, WNOHANG);
  }
}

// Converts the current server process into a background daemon 
void daemon_init(void){
  pid_t pid; 
  struct sigaction act; // Signal handler. 
  
  // Creates child process. Checks for errors. 
  if((pid = fork()) < 0){
    perror("fork"); exit(1);
  } else if(pid != 0) {
    printf("Daemon pid = %d\n", pid);
    exit(0); // Running Parent process is exited. No more foreground processes.  
  }

  // Child process Continues daemon setup 
  setsid(); // Become Session Leader 
  chdir("/"); // Change working directory to root for stability 
  umask(0); // Clear file mode creation mask 
  
  // Removing zombie processes using SIGCHILD and Sigaction 
  act.sa_handler = claim_children; // Assign function pointer for reliable signal
  sigemptyset(&act.sa_mask); // Dont block other signals 
  act.sa_flags = SA_NOCLDSTOP; // Not catch sopped children 
  sigaction(SIGCHLD, (struct sigaction *)&act, (struct sigaction *)0); // When a zombie signal is found, claim children is fired by the signal handler 
}

// Authenticate the clients username and password before giving access to the program 
int authenticate_client(int sd){
  int nr, nw;
  char buf[MAX_BLOCK_SIZE];
  char username[MAX_BLOCK_SIZE]; 
  char password[MAX_BLOCK_SIZE]; 
  Account user;
  initialize_account(&user, "danielle", "hello123");

  // Get and Store Username details 
  nw = writen(sd, "Username: ", strlen("Username: "));
  if((nr = readn(sd, buf, sizeof(buf))) <= 0){
    return 0; // Connection error
  }

  buf[nr] = '\0'; // Change end of string to null terminator 
  strcpy(username, buf); // Ensures no buffer overflow 

  // Get and Store Password details 
  nw = writen(sd, "Password: ", strlen("Password: "));
  if((nr = readn(sd, buf, sizeof(buf))) <= 0){
    return 0; // Connection error
  }

  buf[nr] = '\0';
  strcpy(password, buf);

  // Authenticate Username and Password 
  // 0 = Fail | 1 = Success 
  if(strcmp(username, user.username) != 0 || strcmp(password, user.password) != 0){ 
    nw = writen(sd, "0", 1);
    return 0; 
  }

  // Success Code  
  writen(sd, "1", 1);
  return 1; 
  
}

// Echoes client message appending with "You Said ___". Exits when quit is received. 
// Sd = Socket Descriptor of the current client. One client per child process 
void serve_a_client(int sd){
  int nr, nw; // Read and Write return signals 
  char buf[MAX_BLOCK_SIZE]; // buffer is the size of the max data block size 

  // Authenticate Client 
  if(authenticate_client(sd) == 0){
    close(sd);
    return; 
  }
  
  // Initialize File descriptors to socket 
  // Changes input from keyboard to socket 
  dup2(sd, STDIN_FILENO);
  dup2(sd, STDOUT_FILENO);
  dup2(sd, STDERR_FILENO);

  // Run shell executable for each child. 
  // Execl is used when a full path and name of the executable is known and fixed 
  execl("./shell", "shell", (char *)NULL); 
  perror("execl"); 
  exit(1);
  
}

/**
* Main server loop 
* Optional Improvements 
* - Add status code at each message sent from the server so the client wont need to parse hardcoded strings 
* - 0 = Login failed | 1 = Login Successful | 2 = Normal Message | 3 = Exit program 
* - Use strncpy for copying string to avoid overflow 
* - Use snprintf for echoing user messages to avoid overflow 
* - 
*/ 
int main(){ 
  int sd, nsd, n, cliAddressLen; // Server Socket descriptor, Network socket discovery to find client socket and Client Address length 
  pid_t pid; 
  struct sockaddr_in serverAddress, cliAddress;  

  // Create daemon process 
  daemon_init();
  
  // Setup Listening socket 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("server:socket"); exit(1);
  }

  // Build Server listening socket address
  bzero((char *)&serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET; 
  serverAddress.sin_port = htons(SERVER_TCP_PORT); 
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); 

  // Bind the Server Address to socket sd 
  if(bind(sd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    perror("server:bind"); exit(1);
  }

  // Become listening socket for 5 users only.  
  listen(sd, 5); 
  
  while(1){
    // Wait to accept client request for connection 
    cliAddressLen = sizeof(cliAddress);
    nsd = accept(sd, (struct sockaddr *)&cliAddress, (socklen_t *)&cliAddressLen);
    
    // Check for accept errors 
    if(nsd < 0){
      // If ineterrupted by SIGCHLD 
      if(errno == EINTR){
        continue; 
      }
      perror("server:accept"); exit(1);
    }

    // Create a child process to serve this particular client 
    if((pid = fork()) < 0){
      perror("fork"); exit(1);
    } else if( pid > 0){
      close(nsd); 
      continue; // nsd reset, next client can connect
    }

    // In child, serve the current client 
    close(sd);
    serve_a_client(nsd);
    close(nsd);
    exit(0);
  }
}

