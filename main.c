#include <sys/socket.h> //Funcion del socket
#include <netinet/in.h> //Funcion sockaddr_in
#include <arpa/inet.h> //Funcion htons
#include <unistd.h> //Para la funcion Accept y Close
#include <stdio.h>
#include <stdlib.h> //Libreria para la funcion Error
#include <string.h> //Libreria para la funcion strlen

#define PORT 8080

#define BUFFER_SIZE 256

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
      perror("Error al relacionar el socket!\n");
      exit(EXIT_FAILURE);
    }
    
    //Escucha las conexiones entrantes
    int backlog = 3; //Numero de conexiones autorizadas
  
    if(listen(socket_fd, backlog) < 0 ){
      perror("Error al escuchar!\n");
      exit(EXIT_FAILURE);
    }     
    printf("Escuchando en el puerto 8080...\n"); 
    return socket_fd;
}

void HandleResponse(int code, int ClientSocket){
  char * response;
  if(code == 210){
      response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nhello\n";
  } else if (code == 212){ "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
  }else {
      response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
  }

  write(ClientSocket, response, strlen(response));
  close(ClientSocket);
}

void HandleClient(int ServerSocket, int ClientSocket){
    //Obtener los datos recibidos del socket y los almacena en un buffer
    char buffer [BUFFER_SIZE];
        
    int DatosDelBuffer = recv(ClientSocket, buffer, sizeof(buffer), 0);  
        
    if(DatosDelBuffer < 0 ){
      perror("Error leer el buffer!\n");
      exit(EXIT_FAILURE);
    } else {
      printf("%s\n",buffer);
      
      //Verificamos si lo que hay en el buffer es un GET o un HEADERS
      if(strncmp(buffer,"GET / ",6) == 0){
        HandleResponse(210,ClientSocket);
      } else if(strncmp(buffer,"GET /headers",12) == 0){
        //HandleResponse(212,ClientSocket);
      }
    } 
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
      HandleClient(ServerSocket,ClientSocket);
    }
        
    //Cierra el Socket y termina la conexion
    close(ServerSocket);
    return 0;
}

