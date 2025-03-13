#include <stdio.h>
#include <sys/socket.h> // socket
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons
#include <unistd.h> // Accept , Close
#include <stdlib.h> // perror
#include <string.h> // strlen , strncmp

#define PORT 8080 //int

#define BUFFER_SIZE 256 //int 

int initServerSocket(){
     // Creacion del socket
    // AF_INET = IPv4
    // SOCK_STREAM = TCP
    // 0 = Protocolo por default 
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_fd < 0){
      perror("Error al crear el socket!\n");
      exit(EXIT_FAILURE);
    }

    // Configura la direccion del servidor   
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Relaciona el socket con la direccion
    if(bind(socket_fd,(struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ){
      perror("Error al relacionar el socket! ");
      exit(EXIT_FAILURE);
    }
    
    //Escucha las conexiones entrantes
    int backlog = 3; //Numero de conexiones autorizadas
  
    if(listen(socket_fd, backlog) < 0 ){
      perror("Error al escuchar! ");
      exit(EXIT_FAILURE);
    }     
    printf("Escuchando en el puerto '%d'...\n",PORT); 
    return socket_fd;
}

void HandleHeaders(int ClientSocket, char *request){

  char response[BUFFER_SIZE];
  
  char *header_start = strstr(request,"\r\n");

  if (header_start == NULL){
    //Manejo en cazo que no se encuentre el inicio de los encabezados
    perror("\nError al buscar los encabezados!");
    return; 
  }

  header_start += 2;
  //"\r" y "\n" cuentan como caracteres, sumamos +2 para avanzar dos caracteres y comenzar en la primera
  // linea de los encabezados

  char *header_end = strstr(header_start,"\r\n\r\n");

  if(header_end == NULL){
    perror("\nError al buscar el final de los encabezados!");
    return;
  }
}

void HandleResponse(int ClientSocket, char *request){
  char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHOLA";
  write(ClientSocket, response, strlen(response));
}

void HandleRequest(int ServerSocket, int ClientSocket){
    //Obtener los datos recibidos del socket y los almacena en un buffer
    char request [BUFFER_SIZE]; //Buffer
    
    int DatosDelBuffer = recv(ClientSocket, request, sizeof(request), 0);

    //Verificamos el contenido del buffer
    if(DatosDelBuffer < 0 ){
      perror("Error leer el buffer!\n");
      exit(EXIT_FAILURE);
    } 
    
    request[DatosDelBuffer] = '\0'; //Nos aseguramos que al final de los datos del buffer haya un NULL
    
    HandleHeaders(ClientSocket,request); //Llamamos al metodo que maneja los encabezados antes de mandar una respuesta

    if(strncmp(request,"GET / ",6) == 0){
//      printf("%s\n",request); 
      HandleResponse(ClientSocket,request);
    } else {
      char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
      write(ClientSocket, response, strlen(response)); 
    }    
    
    //Cierra el Socket y termina la conexion
    close(ClientSocket);
}
  
int main() {
    int ServerSocket = initServerSocket();
   
    // Control de las conexiones de los clientes
    while(1){
      //Informacion del cliente
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr); //Size of address buffer
      int ClientSocket = accept(ServerSocket, (struct sockaddr *)&client_addr, &client_len); 
      
      // Acepta la conexion de los clientes
      if(ClientSocket < 0){
        perror("Error al acceptar al cliente!\n");
        exit(EXIT_FAILURE);
      }
      HandleRequest(ServerSocket,ClientSocket);
    }
    return 0;
}
