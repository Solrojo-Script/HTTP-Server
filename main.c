/*HEADERS*/
#include <stdio.h>
#include <sys/socket.h> // socket
#include <sys/types.h> //ssize_t
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons
#include <unistd.h> // Accept , Close
#include <stdlib.h> // perror
#include <string.h> //strstr, strcmp
#include <netinet/tcp.h> //Keep Alive TCP Options
#include <errno.h>

#define PORT 8080 //int
#define BUFFER_SIZE 1000000 //Bytes = 1MB

//Config of the server conection
int init_server_socket(){
   int server_socket = 1; //Server File Descriptor
   struct sockaddr_in server_addr;
   int opt = 1; //Activate option

    //AF_INET = IPv4     SOCK_STREAM = TCP     0 = Default Protocol
     
    // Socket creation
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
      perror("SOCKET ERROR!\n");
      exit(EXIT_FAILURE);
    }

   // Avoid 'Address Already in use' Error
   // Configure socket options. SO_REUSEADDR allows reusing a local address
   // (e.g., after restarting the server while the port is in TIME_WAIT).
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
     perror("SO_REUSEADDR FAILED!");
     return 1;
    }

    // Server configuration   
    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; //Listen on any interface (127.0.. , localhost, etc)
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

int init_client_socket (int cli_sock) { 
    int keepalive_time; //Seconds
    int opt = 1;

    // Enable TCP keepalive probes
    if (setsockopt(cli_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
      perror("Keep-Alive Configuration FAILED!");
      return 1;
    }
    
    keepalive_time = 60;
    // Idle secs
    if (setsockopt(cli_sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepalive_time, sizeof(keepalive_time)) < 0) {
      perror("Keep Alive Iddle Time FAILED!");
      return -1;
    }

    keepalive_time = 10;
    // Probe interval
    if (setsockopt(cli_sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepalive_time, sizeof(keepalive_time)) < 0){
      perror ("Keep Alive Probe Interval FAILED!");
      return -1;
    }

    keepalive_time = 3;
    // Probe count
    if (setsockopt(cli_sock, IPPROTO_TCP, TCP_KEEPCNT, &keepalive_time, sizeof(keepalive_time)) < 0){
      perror ("Keep Alive Probe Count FAILED!");
      return -1;
    }
    
    return cli_sock;
}

char * handle_response( char * method, char * path){
  char *status,*keep_alive,*content_type,*content;
  //strcmp to verify the method
 if (strcmp(method,"GET") == 0){
    if (strcmp(path,"/") == 0){
      status = "HTTP/1.1 200 OK";
      keep_alive = "Connection: Keep-Alive";
      content_type = "Content-Type: text/plain";
      content = "HELADO";
    } else {
      status = "HTTP/1.1 404 Not Found";
      content_type = "Content-Type: text/plain";
      content = "404 Not Found";
    }
  } else {
  // DO SOMETHING ELSE IF IS NOT THE GET METHOD
  }

  // Calculates the response size and allocates memory for the HTTP Header response.
  ssize_t len = strlen(status) + strlen(keep_alive) + strlen(content_type) + strlen(content) + strlen("\r\n") + strlen("\r\n") + strlen("\r\n\r\n") + strlen("\n");
  
  char * response = malloc(len);
  snprintf(response, len, "%s\r\n%s\r\n%s\r\n\r\n%s\n", status, keep_alive, content_type, content);
  
  return response;
}


int http_parser(int client_socket, char * request) {
  char buffer[BUFFER_SIZE];

  //Copy the request to a buffer with strlcpy() cus strtok() modifies the original string
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

  const char * response = handle_response(method, path);

  send(client_socket, response, strlen(response), 0);
  return 1;
} 

void handle_client(int client_socket){
  char buffer [BUFFER_SIZE]; //buffer for client request

  //Writes the request in the buffer and counts the bytes with ssize_t
  ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) -1, 0);
  
  // Close the socket if no data was received or an error occurred.
  if (bytes_received <= 0){
    close(client_socket);
  }

  buffer[bytes_received] = '\0'; //Add a null terminator so the buffer can be treated as a C string.

  printf("Request:\n %s \n", buffer);
  
  if(bytes_received > 0){
    http_parser(client_socket, buffer);
  } 

  //Commented out to keep the socket open to allow HTTP keep-alive connections.
  //close(client_socket); 

}

int main() {
  int server_socket = init_server_socket();
 
  // Cient connections control
  while(1){
    //Client info
    struct sockaddr_in client_addr; //IP address and port of the client
    socklen_t client_addr_len = sizeof(client_addr); //Size of address buffer
    int client_socket,cli_sock;
    
    //Creates de Client Socket
    if ((client_socket = accept(server_socket,
                                 (struct sockaddr *)&client_addr,
                                 &client_addr_len)) < 0){
      perror("Failed to accept the client!\n");
      continue;
    }

    // Client Socket Configuration
    cli_sock = init_client_socket(client_socket);

    //Handles the Client's Request
    handle_client(cli_sock);
  }
    close(server_socket);
    return 0;
}
