#include <sys/socket.h> //Funcion del socket
#include <netinet/in.h> //Funcion sockaddr_in
#include <arpa/inet.h> //Funcion htons
#include <unistd.h> //Para la funcion Accept y Close
#include <stdio.h>
#include <stdlib.h> //Libreria para la funcion Error

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
    server_addr.sin_port = htons(8080);
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

void HandleClient(int ServerSocket, int ClientSocket){
      while(1){
        //Obtener los datos recibidos del socket y los almacena en un buffer
        unsigned char * buffer [256]; //Buffer que espera datos en binario
        
        int DatosBuffer = recv(ClientSocket, buffer, sizeof(buffer),0);  
        
        if(DatosBuffer < 0 ){
          perror("Error leer el buffer!\n");
          exit(EXIT_FAILURE);
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

