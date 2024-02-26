#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define FILE_SIZE_MB 2
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // Argument validation
    // ...

    const char *RECIVER_IP = argv[1];
    int RECIVER_PORT = atoi(argv[2]);
    const char *congestion_algorithm = argv[3];


    // Read the created file
    char *filename[BUFFER_SIZE];
    for(int i=0; i<BUFFER_SIZE; i++){
    scanf(" %s", &filename[i]);}


    FILE *file = fopen("your_filename.txt", "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }


    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error creating socket");
        fclose(file);
        exit(EXIT_FAILURE);
    }


    // struct of socket
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));



    // Set up connection parameters
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(RECIVER_PORT);
	int rval = inet_pton(AF_INET, (const char*)RECIVER_IP, &serverAddress.sin_addr);
	if (rval <= 0)
	{
		printf("inet_pton() failed");
		return -1;
	}


    // Connect to the receiver
    // Make a connection to the server with socket SendingSocket.

     if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
     {
	   printf("connect() failed with error code");} 
       //if the connection secceed  
       printf("connected to server\n");


    // Send the file 
    // ------ PROBLEM: CHECK IF THE BUFFER_SIZE IS THE INPUT
    int bytesSent = send(sock, file, BUFFER_SIZE, 0);

     if (-1 == bytesSent)
     {
	printf("send() failed with error code"); }

     else if (0 == bytesSent)
     {
	printf("peer has closed the TCP connection prior to send().\n");}

     else if (BUFFER_SIZE > bytesSent)
     {
	printf("sent only %d bytes from the required %d.\n", BUFFER_SIZE, bytesSent);}

     else 
     {
	printf("message was successfully sent .\n");}
    

    // User decision: Send the file again or exit ---- ??? 
    Sleep(3000);

    // Send exit message to the receiver
    // ...

    // Close the TCP connection
    closesocket(sock);

    // Exit
    close(sock);

    return 0;
}
