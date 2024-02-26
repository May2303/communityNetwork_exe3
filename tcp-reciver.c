#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 5060 //PROBLEM: I NEED TO FIX THIS!!

int main(int argc, char *argv[]) {

    int port = atoi(argv[1]);
    const char *congestion_algorithm = argv[2];
    int enableReuse = 1;
    int listeningSocket = -1; 

    signal(SIGPIPE, SIG_IGN);
    
    
    if((listeningSocket = socket(AF_INET , SOCK_STREAM , 0 )) == -1){
        printf("Could not create listening socket : %d" ,errno);
    }

    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0){
         printf("setsockopt() failed with error code : %d" , errno);
    }


    // "sockaddr_in" is the "derived" from sockaddr structure
     struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);  //network order


    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a specific port
    if (bind(listeningSocket, 
            (struct sockaddr *)&serverAddress, 
            sizeof(serverAddress)) == -1){
        printf("Bind failed with error code : %d" , errno);
      //close the socket:
      close(listeningSocket);
     return -1;
    
    printf("Bind() success\n");
    }
    
    // Listen for incoming connections
    if (listen(listeningSocket, 500) == -1){ //500 is a Maximum size of queue connection requests
	   printf("listen() failed with error code");
       close(listeningSocket);
         return -1;
}
    
    // Accept a connection from the sender
    printf("Waiting for incoming TCP-connections...\n");

    // Receive the file, measure the time, and save it
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);

    while (1)
    {
    	memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
    	if (clientSocket == -1)
    	{
           printf("listen failed with error code");
	   //close the sockets
        close(listeningSocket);
           return -1;
    	}
      
    	printf("A new client connection accepted\n");

    }

   



    // Print out times and average bandwidth
    // ...

    // Calculate average time and total average bandwidth
    // ...

    // Close the TCP connection
    close(listeningSocket);

    // Exit
    // ...

    return 0;
}
