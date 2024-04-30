#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>

// Define RUDP flags
#define RUDP_SYN 0x01
#define RUDP_ACK 0x02
#define RUDP_FIN 0x04
#define RUDP_DATA 0x08

// Define packet size
#define PACKET_SIZE 1024

// Define maximum number of retries
#define MAX_RETRIES 5

// Define file name for received data
#define RECEIVED_FILE "received_data.bin"

//void print_statistics(clock_t start_time, clock_t end_time);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse port number
    int port = atoi(argv[1]);

    // Create UDP connection + handshake
    int sock;
    if (rudp_socket(port, &sock) == -1) {
        perror("Error creating UDP connection");
        exit(EXIT_FAILURE);
    }

    // Get connection from sender (handshake)
    struct sockaddr_in sender_addr;

    // Receive file and measure time


    // Wait for sender response
    char decision;
    while (1) {
        printf("Waiting for sender response...\n");

        // Receive response from sender
        if (recvfrom(sock, &decision, sizeof(decision), 0, (struct sockaddr *)&sender_addr, sizeof(sender_addr)) == -1) {
            perror("Error receiving response from sender");
            rudp_close(sock);
            exit(EXIT_FAILURE);
        }

        // Handle response
        if (decision == 'y') {
            printf("Sender wants to resend the file.\n");
            if (rudp_recv(sock, sender_addr) == -1) {
                perror("Error receiving file");
                rudp_close(sock);
                exit(EXIT_FAILURE);
            }
        } else if (decision == 'n') {
            printf("Sender wants to exit.\n");
            break;
        } else {
            printf("Invalid response from sender.\n");
        }
    }

    // Print out statistics and exit
    // ...

    // Close socket
    rudp_close(sock);


    // Close file

    return 0;
}


