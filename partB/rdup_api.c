#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>

#define RUDP_DEF 0x00 // Standard data transfering packet flag
#define RUDP_SYN 0x01 // Sync flag
#define RUDP_ACK 0x02 // Acknowledgement flag
#define RUDP_FIN 0x04 // Ending program/connection flag
#define PACKET_SIZE 1024
#define MAX_SEQ_NUM 1000 // Maximum sequence number expected

typedef struct {
    uint16_t length;
    uint16_t checksum;
    uint8_t flags;
} RUDP_Header;

/* 
Function to calculate the one's complement checksum of binary data
Parameters:
data: Pointer to the binary data buffer
packet_length: Length of the data buffer in bytes
Iterates over the data and sums it in 16bit and folds the sum. 
Returns:
    The complement of sum
*/

uint16_t calculate_checksum(const uint8_t *data, int packet_length) {
    uint32_t sum = 0; // Initialize a 32-bit sum variable
    
    // Iterate over each 16-bit word of the data (assuming packet_length is even)
    for (int i = 0; i < packet_length / 2; i++) {
        // Combine two bytes into a 16-bit word and add it to the sum
        sum += ((uint16_t)data[2*i] << 8) + data[2*i+1];
    }

    // If packet_length is odd, add the last byte as a padded 16-bit word
    if (packet_length % 2) {
        sum += ((uint16_t)data[packet_length - 1] << 8);
    }

    // Fold the carry bits into the sum
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Take the one's complement of the sum
    uint16_t checksum = ~sum; // Calculate one's complement checksum

    return checksum; // Return the calculated checksum
}


// Function to handle timeouts during data transmission
int handle_timeout(int sockfd) {
    // Implement timeout handling logic
    // ...
}

// Function to send a packet with retries
int send_with_retries(int sockfd, const char *receiver_ip, int port, const char *data, int length) {
    // Implement sending logic with retries and timeout handling
    // ...
}

bool detect_packet_loss(const int *ack_seq_numbers, int num_acks) {
    bool packet_loss = false;
    bool received[MAX_SEQ_NUM] = { false };

    // Mark received sequence numbers
    for (int i = 0; i < num_acks; ++i) {
        if (ack_seq_numbers[i] >= 0 && ack_seq_numbers[i] < MAX_SEQ_NUM) {
            received[ack_seq_numbers[i]] = true;
        }
    }

    // Check for missing sequence numbers
    for (int i = 0; i < MAX_SEQ_NUM; ++i) {
        if (!received[i]) {
            printf("Packet with sequence number %d is missing.\n", i);
            packet_loss = true;
        }
    }

    return packet_loss;
}


void rudp_socket(int *sockfd, int port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(*sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(*sockfd);
        exit(EXIT_FAILURE);
    }

}

int rudp_send(int sockfd, const char *receiver_ip, int port, const char *file_path, long fileSize) {
    // Implement RUDP data sending logic
    // ...
}

void rudp_recv(int sockfd, const char *file_path) {
    // Implement RUDP data receiving logic
    // ...
}

void rudp_close(int sockfd) {
    close(sockfd);
}
