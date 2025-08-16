/*HEADERS*/
#include <stdio.h>
#include <sys/socket.h> // socket
#include <sys/types.h> //ssize_t
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons
#include <unistd.h> // Accept , Close
#include <stdlib.h> // perror
#include <string.h> //strstr, strcmp
#include <errno.h>

#define PORT 8080 //int
#define BUFFER_SIZE 1000000 //Bytes = 1MB

int init_server_socket(){
   int server_socket; //Server File Descriptor
   struct sockaddr_in server_addr;
     
    //AF_INET = IPv4     SOCK_STREAM = TCP     0 = Default Protocol
     
    // Socket creation
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
      perror("SOCKET ERROR!\n");
      exit(EXIT_FAILURE);
    }

   // Avoid 'Address Already in use' Error
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
     printf("SO_REUSEADDR FAILED! %s\n",strerror(errno));
     return 1;
    }

    // Server configuration   
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to the server's address and port
    if(bind(server_socket,(struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ){
      perror("BIND ERROR!");
      exit(EXIT_FAILURE);
    }
    
    //Listen for incoming connections
    int connections = 10; //Max connections
  
    if(listen(server_socket, connections) < 0 ){
      perror("LISTEN ERROR! ");
      exit(EXIT_FAILURE);
    }     
    printf("Listening on port '%d'...\n",PORT);
    return server_socket;
}


char * handle_response( char * method, char * path, char * protocol){
  char * response;
  
  //strcmp to verify the method
 if (strcmp(method,"GET") == 0 && strcmp(path,"/") == 0){
      response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nhello\n";
 } else {
      response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
  }

  return response;
}


int http_parser(int client_socket, char * request) {
  char buffer[BUFFER_SIZE];

  //Copy the request to the buffer
  strlcpy(buffer,request, sizeof(buffer));

  //METHOD
  char * method = strtok(buffer, " ");
  if(method == NULL){
   printf("Invalid Request Method!\n"); 
   return 0;
  }

  //PATH
  char * path = strtok(NULL, " ");
  if (path == NULL){
    printf("Invalid Request Path\n");
    return 0;
  }

  //PROTOCOL
  char * protocol = strtok(NULL,"\r\n");
  if(protocol == NULL){
    printf("Invalid Request Protocol\n");
    return 0;
  }

  const char * response = handle_response(method, path, protocol);

  send(client_socket, response, strlen(response), 0);
  return 1;
} 

void handle_client(int client_socket){
  char buffer [BUFFER_SIZE]; //buffer for client request

  //Writes the request in the buffer and counts the bytes with ssize_t
  ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) -1, 0);
  
  buffer[bytes_received] = '\0'; //NULL Terminate the request

  printf("Request:\n %s \n", buffer);
  
  if(bytes_received > 0){
    http_parser(client_socket, buffer);
  } 
  close(client_socket);
}

int main() {
  int server_socket = init_server_socket();
 
  // Cient connections control
  while(1){
    //Client info
    struct sockaddr_in client_addr; //IP address and port of the client
    socklen_t client_addr_len = sizeof(client_addr); //Size of address buffer
    int client_socket;
    
    if ((client_socket = accept(server_socket,
                                 (struct sockaddr *)&client_addr,
                                 &client_addr_len)) < 0){
      perror("Failed to accept the client!\n");
      continue;
    }
    handle_client(client_socket);
  }
    close(server_socket);
    return 0;
}
