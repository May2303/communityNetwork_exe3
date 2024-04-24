// RUDP_Sender.c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>


#define RUDP_SYN 0x01
#define RUDP_ACK 0x02
#define RUDP_FIN 0x04
#define PACKET_SIZE 1024

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5060
#define BUFFER_SIZE 100

#define FILE_SIZE 2 * 1024 * 1024 // 2 Megabytes buffer size
#define BUFFER_SIZE  2 * 1024 // Size of packets

void generate_random_file(const char *filename, size_t size) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error opening file for writing\n");
        return;
    }

    char *buffer = (char *)malloc(size);
    if (buffer == NULL) {
        printf("Error allocating memory for buffer\n");
        fclose(file);
        return;
    }

    // Generate random data and write to file
    for (size_t i = 0; i < size; i++) {
        buffer[i] = rand() % 256; // Generate random byte
    }
    fwrite(buffer, 1, size, file);

    free(buffer);
    fclose(file);
}

void print_statistics(clock_t start_time, clock_t end_time) {
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time taken: %.2f ms\n", (elapsed_time * 1000));
}


int main(int argc, char *argv[]) {
    
    if (argc != 4 || strcmp(argv[1], "-p") != 0) {
        printf("Usage: %s -p <receiver_port>\n", argv[0]);
        return -1;
    }

    const char *receiver_ip = argv[2];
    int receiver_port = atoi(argv[4]);
    const char *file_name = "random_file.bin";

    // Generate random file of at least 2MB size
    size_t file_size_bytes = FILE_SIZE;
    generate_random_file(file_name, file_size_bytes);

     // Create a RUDP socket
    int sock = -1;
    if ((sock = rudp_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
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
	 if (rudp_send(sock, receiver_ip, receiver_port, fileContent, fileSize) == -1) {
        printf("Error in sending file using RUDP\n");
        free(fileContent);
        close(sock);
        return -1;
    }


    struct sockaddr_in fromAddress;
	//Change type variable from int to socklen_t: int fromAddressSize = sizeof(fromAddress);
	socklen_t fromAddressSize = sizeof(fromAddress);
	memset((char *)&fromAddress, 0, sizeof(fromAddress));

    char bufferReply[BUFFER_SIZE]; // Define the buffer with a predefined size

	// try to receive some data, this is a blocking call
	if (rudp_recv(sock, bufferReply, sizeof(bufferReply) -1, 0, (struct sockaddr *) &fromAddress, &fromAddressSize) == -1){
		printf("recvfrom() failed with error code  : %d", errno);
        return 1;
    }
    printf(bufferReply);

  

    // User decision: Send the file again?
    char decision;
    char buffer[BUFFER_SIZE];

    while (1) {
    if (decision == 'y' || decision == "Y")
        printf("Continuing...\n");

    printf("Sending %ld bytes of data...\n", fileSize);
    
    // Open the file for reading
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("Error opening file for reading");
        rudp_close(sock);
        return -1;
    }

    // Start the clock
    clock_t start_time = clock();

    size_t total_bytes_sent = 0;
    size_t bytes_read;

    // Loop until the entire file is sent
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // Send the data using RUDP
        if (rudp_send(sock, receiver_ip, receiver_port, buffer, bytes_read) == -1) {
            perror("Error sending file");
            fclose(file);
            rudp_close(sock);
            return -1;
        }
        total_bytes_sent += bytes_read;
    }

    // Stop the clock
    clock_t end_time = clock();

    fclose(file);
    printf("Successfully sent %ld bytes of data!\n", total_bytes_sent);

    // Calculate and print statistics
    print_statistics(start_time, end_time);

    // Ask user if they want to send more data
    printf("Do you want to send more data? (y/n): ");
    scanf(" %c", &decision);

    while ((decision != 'y' || decision == "Y") && (decision != 'n' || decision == "N")) {
        printf("Invalid answer, do you want to send more data? (y/n) ");
        scanf(" %c", &decision);
    }

    // Send the decision to the receiver
    if (rudp_send(sock, &decision, sizeof(decision), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error sending decision");
        rudp_close(sock);
        return -1;
    }

    if (decision == 'n' || decision == "N")
        break;
    else if (decision == 'y' || decision == "Y") {
        printf("Waiting for the server to be ready...\n");

        // Receive the sync byte from the receiver
        char sync_byte;
        ssize_t bytes_received = rudp_recv(sock, &sync_byte, sizeof(sync_byte), 0);
        if (bytes_received < 0) {
            perror("Error receiving sync byte from receiver");
            rudp_close(sock);
            return -1;
        } else if (bytes_received == 0) {
            printf("Server %s:%d disconnected.\n", receiver_ip, receiver_port);
            rudp_close(sock);
            return 0;
        }

        // Check if the received byte is the sync byte
        if (sync_byte == 'S') {
            printf("Server accepted your decision.\n");
        } else {
            printf("Error: Unexpected response from server.\n");
            rudp_close(sock);
            return -1;
        }
    }
}

printf("Closing connection...\n");

// Close the RUDP connection
rudp_close(sock);
    free(fileContent);

    // Exit
    return 0;
}
