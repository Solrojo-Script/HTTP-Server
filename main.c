/* Standard Library and Socket Headers */
#include <stdio.h>      // printf, perror, fprintf
#include <stdlib.h>     // malloc, free, exit
#include <string.h>     // strlen, strcmp, strtok, strncpy
#include <unistd.h>     // close
#include <sys/socket.h> // socket, bind, listen, accept, recv, send
#include <sys/types.h>  //ssize_t
#include <netinet/in.h> // sockaddr_in, htons
#include <netinet/tcp.h>// TCP keepalive options
#include <arpa/inet.h>  // inet functions
#include <errno.h>      // errno values

#define PORT 8080 //int
#define BUFFER_SIZE 1000000 //Bytes = 1MB
#define KEEPALIVE_REQUEST_MAX 50

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
    int keepalive_time, keepalive_probe, keepalive_cnt;
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

    keepalive_probe = 10;
    // Probe interval
    if (setsockopt(cli_sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepalive_probe, sizeof(keepalive_probe)) < 0){
      perror ("Keep Alive Probe Interval FAILED!");
      return -1;
    }

    keepalive_cnt = 3;
    // Probe count
    if (setsockopt(cli_sock, IPPROTO_TCP, TCP_KEEPCNT, &keepalive_cnt, sizeof(keepalive_cnt)) < 0){
      perror ("Keep Alive Probe Count FAILED!");
      return -1;
    }
    
    return cli_sock;
}

char * handle_response( char * method, char * path){
  size_t offset = 0;
  int len = 0;
  char *status = "";
  char *content = "";
  char *content_type = "";
  char *keep_alive = "Connection: Keep-Alive";
  char *keep_alive_time = "Keep-Alive: timeout=10, max=50";
  char *allowed_method = "";
  char content_length [32];

  if (method == NULL || path == NULL){
      status = "HTTP/1.1 400 Bad Request";
      keep_alive = "Connection: Close";
      keep_alive_time = "";
  } else {
    // Validate HTTP method and requested path
    if (strcmp(method,"GET") == 0){
      if (strcmp(path,"/") == 0){
        status = "HTTP/1.1 200 OK";
        content_type = "Content-Type: text/plain";
        content = "Ice Cream !";
      } else {
        status = "HTTP/1.1 404 Not Found";
        content_type = "Content-Type: text/plain";
        content = "404 Not Found";
      }
    } else {
      // DO SOMETHING ELSE IF IS NOT THE GET METHOD
      status = "HTTP/1.1 405 Method Not Allowed";
      allowed_method = "Allow: GET";
      content_type = "Content-Type: text/plain";
      content = "Method Not Allowed";
    }
  } 

  // Verifies if the content length is not negative
  int c_length = snprintf(content_length, sizeof(content_length),"Content-Length: %zu", strlen(content));
  if ( c_length< 0){
    fprintf(stderr, "Error in Content-Length header\n");
    return NULL;
  } else if ((size_t)c_length >= sizeof(content_length)){
    fprintf(stderr, "Truncation in Content-Length header\n");
    return NULL;
  }

  // Build the HTTP response and return the allocated buffer
  len += snprintf(NULL, 0, "%s\r\n",status);
  len += snprintf(NULL, 0, "%s\r\n",keep_alive);
  len += snprintf(NULL, 0, "%s\r\n",keep_alive_time);

  if (allowed_method[0] != '\0'){
    len += snprintf(NULL, 0, "%s\r\n",allowed_method);
  }
  
  len += snprintf(NULL, 0, "%s\r\n",content_type);
  len += snprintf(NULL, 0, "%s\r\n\r\n",content_length);
  len += snprintf(NULL, 0, "%s",content);

  //If snprintf failed to during formating
  if (len < 0 ){
   return NULL;
  }

  //If failed to allocate memory
  char * response = malloc(len + 1);
  if (response == NULL){
   return NULL;
  }

  // Build HTTP response headers and body
  //snprintf(response, len + 1,  "%s\r\n" "%s\r\n" "%s\r\n" "%s\r\n" "%s\r\n\r\n" "%s", status, keep_alive, keep_alive_time, allowed_method, content_type, content_length, content);
  offset += snprintf(response + offset,len + 1 - offset, "%s\r\n",status);
  offset += snprintf(response + offset,len + 1 - offset, "%s\r\n",keep_alive);
  offset += snprintf(response + offset,len + 1 - offset, "%s\r\n",keep_alive_time);
  
  // Add Allow header only for 405 responses
  if (allowed_method[0] != '\0'){
    printf("Added allowed Method");
    offset += snprintf(response + offset,len + 1 - offset, "%s\r\n",allowed_method);
  }

  offset += snprintf(response + offset,len + 1 - offset, "%s\r\n",content_type);
  offset += snprintf(response + offset,len + 1 - offset, "%s\r\n\r\n",content_length);
  offset += snprintf(response + offset,len + 1 - offset, "%s",content);
  return response;
}


int http_parser(int client_socket, char * request) {
  char buffer[BUFFER_SIZE];

  //Copy the request because strtok() modifies the original string
  strncpy(buffer,request, sizeof(buffer)-1);
  buffer[sizeof(buffer)-1] = '\0';

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

  if(response == NULL){
    fprintf(stderr,"Failed to create HTTP Response\n");
    return 0;
  }
  //DEBUG
  printf("----- RESPONSE -----\n%s\n--------------------\n", response);

  ssize_t sent = send(client_socket, response, strlen(response), 0);

  if(sent < 0){
    perror("send failed");
  }

  free(response);
  return 1;
} 

void handle_client(int client_socket){
  char buffer [BUFFER_SIZE]; //buffer for client request
  int request_count = 0;

  while (1){

    if (request_count >= KEEPALIVE_REQUEST_MAX){
      printf("Maximum Requests Reached!");
      break;
    }
    
    // Receive client data and store it in the buffer
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) -1, 0);

  // Handle client disconnection or receive errors
    if (bytes_received == 0){
      printf ("The client closed the conection\n");
      break;
    } else if (bytes_received < 0 ){
      perror("recv failed");
      break;
    }

    buffer[bytes_received] = '\0'; //Add a null terminator so the buffer can be treated as a C string.

    
    //DEBUG
    printf("Request #%d:\n%s\n", request_count + 1, buffer);
    
    if(http_parser(client_socket, buffer) == 0){
      fprintf (stderr,"Closing client connection...\n");
      break;
    }

    request_count++;
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
    
     //Creates the Client Socket
    int client_socket = accept(server_socket,(struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket < 0){
      perror("Failed to accept the client!\n");
      continue;
    }

    // TCP Keepalive Configuration
    if (init_client_socket(client_socket) < 0)
    {
      close(client_socket);
      continue;
    }

    //Handles the Client's Request
    handle_client(client_socket); 
  }
    close(server_socket);
    return 0;
}
