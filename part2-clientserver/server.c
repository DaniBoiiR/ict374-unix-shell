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
int authenticate_client(int sd, int numAccounts, Account accounts[]){
  int nr, nw;
  char buf[MAX_BLOCK_SIZE];
  char username[MAX_BLOCK_SIZE]; 
  char password[MAX_BLOCK_SIZE]; 
  Account user;
  initialize_account(&user, "test", "test");

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
  for (int i = 0; i < numAccounts; i++) {
    // Scan the array of structs to see if the entered username and password pair match any
    if(strcmp(username, accounts[i].username) == 0){ 
      if (strcmp(password, accounts[i].password) == 0) {
        nw = writen(sd, "1", 1);
        return 1; 
      }
    }
  }
    /*if(strcmp(username, user.username) != 0 || strcmp(password, user.password) != 0){ 
      nw = writen(sd, "0", 1);
      return 0; 
    }*/
  /*
  // Success Code  
  writen(sd, "1", 1);
  return 1; 
  */

  // Failure Code
  nw = writen(sd, "0", 1);
  return 0; 
}

// Echoes client message appending with "You Said ___". Exits when quit is received. 
// Sd = Socket Descriptor of the current client. One client per child process 
void serve_a_client(int sd, char *workingDir, int numAccounts, Account accounts[]){

  // Authenticate Client 
  if(authenticate_client(sd, numAccounts, accounts) == 0){
    close(sd);
    return; 
  }
 
  // Initialize pipes used for the input and output constant streams
  // These pipes will be running concurrently and are handled by seperate child processes 
  int stdinPipe[2];
  int stdoutPipe[2];
  pipe(stdinPipe);
  pipe(stdoutPipe); 
  pid_t pid = fork(); // Create child process to shell output 

  // Child process is replaced by the shell 
  if(pid == 0){
    close(stdinPipe[1]); // Close write end of the input pipe 
    close(stdoutPipe[0]); // Close read end of the output pipe
    close(sd); 
    
    // Replace stdin, stdout, and stderr to the corrensponding pipes 
    dup2(stdinPipe[0], STDIN_FILENO); // Input is read from the read end of input pipe, Client writes in write end 
    dup2(stdoutPipe[1], STDOUT_FILENO); // Output is written to the write end of output pipe, client reads from the read end. 
    dup2(stdoutPipe[1], STDERR_FILENO); // Error sent in the output pipe 
    
    close(stdinPipe[0]); // Close original pipes, only duplicates will be used  
    close(stdoutPipe[1]);

    chdir(workingDir); // Change working directory to current working directory
    setenv("PATH", "/usr/bin:/bin", 1); 
    write(STDOUT_FILENO, "BEFORE EXEC\n", 12);
    execl("./shell", "shell", NULL); // Child is replaced by shell 
    perror("shell");
    exit(1); 
  } 

  // Parent Process handles input and output redirection to the client. 
  close(stdinPipe[0]); 
  close(stdoutPipe[1]); 
  pid_t sendToClient = fork(); // Child process for relaying data to/from the client 
  
  // In child process, recieve shell output and send to the client 
  if(sendToClient == 0){ 
    char buf[MAX_BLOCK_SIZE];
    int n;

    close(stdinPipe[0]);
    close(stdinPipe[1]); 
    close(stdoutPipe[1]); 

    // Read the data from the output pipe and store inside of buffer 
    while((n = read(stdoutPipe[0], buf, sizeof(buf))) > 0){
      //printf("SERVER GOT %d BYTES: ", n);
      if(write(sd, buf, n) != n){
        break; 
      }
    }

    close(stdoutPipe[0]);
    close(stdoutPipe[0]);
    close(stdoutPipe[1]);
    shutdown(sd, SHUT_WR);
    exit(0); 
  }

  // In parent, recieve client input and send to the shell 
  else {
    char buf[MAX_BLOCK_SIZE];
    int n; 

    while(1){
      // readn() MUST match with the writen() used by the client 
      // otherwise buffer will be filled with garbage data 
      // Required for the protocol to work 
      int n = readn(sd, buf, sizeof(buf));

      if (n <= 0)
          break;

      write(stdinPipe[1], buf, n);
    }
  }

  // Clean up zombie processes from shell and input/output child processes
  close(sd); 
  waitpid(pid, NULL, 0); 
  waitpid(sendToClient, NULL, 0); 
}

/**
* Main server loop 
*/ 
int main(){ 
  int sd, nsd, n, cliAddressLen; // Server Socket descriptor, Network socket discovery to find client socket and Client Address length 
  pid_t pid; 
  struct sockaddr_in serverAddress, cliAddress;  
  char workingDir[1024]; // Working directory of the program. Used as reference to get shell path 
  getcwd(workingDir, sizeof(workingDir)); 

  // Accounts
  int numAccounts = 10;
  Account accounts[numAccounts];
  initialize_account(&accounts[0], "test", "test");
  initialize_account(&accounts[1], "test2", "test2");


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
    serve_a_client(nsd, workingDir, numAccounts, accounts);
    close(nsd);
    exit(0);
  }
}

