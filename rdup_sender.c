// RUDP_Sender.c
#include "stdio.h"
#include "rdup_api.c"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define RUDP_SYN 0x01
#define RUDP_ACK 0x02
#define RUDP_FIN 0x04
#define PACKET_SIZE 1024

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5060
#define BUFFER_SIZE 100

int main(int argc, char *argv[]) {
    
    
    int sock = -1;
    char bufferReply[80] = {'\0'};


    const char *receiver_ip = argv[2];
    int port = atoi(argv[4]);

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        printf("Could not create socket : %d", errno);}
        return -1;


    // Setup the server address structure.
	// Port and IP should be filled in network byte order (learn bin-endian, little-endian)
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);
	int rval = inet_pton(AF_INET, (const char*)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
	if (rval <= 0)
	{
		printf("inet_pton() failed");
		return -1;
	}
   
 /* OPTIONAL??
    rudp_socket(&sock, port);
    */
    // Read the created file
    char filename[BUFFER_SIZE];
    printf("Enter the filename: ");
    scanf("%s", filename);

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Could not open file: %s\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fileContent = (char *)malloc(fileSize);
    if (fileContent == NULL) {
        printf("Memory allocation failed");
        fclose(file);
        return -1;
    }

    fread(fileContent, 1, fileSize, file);
    fclose(file);

  // Read the file and send it using RUDP
	 if (rudp_send(sock, receiver_ip, port, fileContent, fileSize) == -1) {
        printf("Error in sending file using RUDP\n");
        free(fileContent);
        close(sock);
        return -1;
    }


    struct sockaddr_in fromAddress;
	//Change type variable from int to socklen_t: int fromAddressSize = sizeof(fromAddress);
	socklen_t fromAddressSize = sizeof(fromAddress);
	memset((char *)&fromAddress, 0, sizeof(fromAddress));


	// try to receive some data, this is a blocking call
	if (recvfrom(sock, bufferReply, sizeof(bufferReply) -1, 0, (struct sockaddr *) &fromAddress, &fromAddressSize) == -1){
		printf("recvfrom() failed with error code  : %d", errno);
        return 1;
    }
    printf(bufferReply);

  

    // User decision: Send the file again?
    char userDecision;
    do {
        printf("Do you want to send the file again? (y/n): ");
        scanf(" %c", &userDecision);

        if (userDecision == 'y' || userDecision == 'Y') {
            // Send the file content using RUDP
            if (rudp_send(sock, receiver_ip, port, fileContent, fileSize) == -1) {
                printf("Error in sending file using RUDP\n");
                free(fileContent);
                close(sock);
                return -1;
            }
        } else if (userDecision == 'n' || userDecision == 'N') {
            // Send an exit message to the receiver
            const char *exitMessage = "EXIT";
            int exitMessageLen = strlen(exitMessage) + 1;

            if (sendto(sock, exitMessage, exitMessageLen, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
                printf("sendto() failed with error code: %d\n", errno);
                free(fileContent);
                close(sock);
                return -1;
            }
        } else {
            printf("Invalid input. Please enter 'y' for yes or 'n' for no.\n");
        }
    } while (userDecision != 'n' && userDecision != 'N');


    // Send an exit message to the receiver

    // Close the RUDP connection
    rudp_close(sock);
    free(fileContent);

    // Exit
    return 0;
}
