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
    char filename[BUFFER_SIZE];
    printf("Enter the filename: ");
    scanf("%s", filename);


    FILE *file = fopen(filename, "rb");
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


    // Set up connection parameters
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(RECIVER_PORT);
	int rval = inet_pton(AF_INET, (const char*)RECIVER_IP, &serverAddress.sin_addr);
	if (rval <= 0)
	{
		printf("inet_pton() failed");
        close(sock);
		exit(EXIT_FAILURE);
	}


    // Connect to the receiver
     if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1){
	   printf("connect() failed with error code");
       fclose(file);
       close(sock);
       exit(EXIT_FAILURE);
    } 
       
  //if the connection secceed  
   printf("connected to server\n");

     while (1) {
        // Read the created file
        char filename[BUFFER_SIZE];
        printf("Enter the filename: ");
        scanf("%s", filename);

        FILE *file = fopen(filename, "rb");
        if (file == NULL) {
            perror("Error opening file");
            // Continue to the next iteration of the loop
            continue;
        }

        // Send the file
        char buffer[BUFFER_SIZE];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            int bytesSent = send(sock, buffer, bytesRead, 0);
            if (bytesSent == -1) {
                perror("send() failed");
                break;
            }
        }

        // User decision: Send the file again or exit
        printf("Do you want to send another file? (y/n): ");
        char decision;
        scanf(" %c", &decision);

        if (decision != 'y' && decision != 'Y') {
            // If the user enters something other than 'y' or 'Y', exit the loop
            break;
        }

        // Close the current file
        fclose(file);
    }

    // Close the TCP connection
    close(sock);

    return 0;
}
