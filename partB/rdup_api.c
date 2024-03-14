#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

//Flags:
#define RUDP_DEF 0x00 // Standard data transfering packet flag
#define RUDP_SYN 0x01 // Sync flag
#define RUDP_ACK 0x02 // Acknowledgement flag
#define RUDP_FIN 0x04 // Ending program/connection flag


#define PACKET_SIZE 1024 // Packet size

typedef struct {
    uint16_t length;
    uint16_t checksum;
    uint8_t flag;
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

void rudp_socket(int *sockfd, int port) {
    // Create a UDP socket and bind to the specified port
    // ...
}

// Function to send data over RUDP connection with custom header
int rudp_send(const uint8_t *data, int data_length, uint8_t flag, int sockfd, struct sockaddr *dest_addr, socklen_t addrlen) {
    // Construct the RUDP header
    RUDP_Header header;
    header.length = data_length; // Set the length field
    header.flag = flag; // Set the flag field
    
    // Calculate checksum for the data (you need to implement this function)
    header.checksum = calculate_checksum(data, data_length);
    
    // Allocate memory for the packet buffer
    uint8_t *packet = malloc(sizeof(RUDP_Header) + data_length);
    if (packet == NULL) {
        return -1; // Return error code if memory allocation failed
    }
    
    // Copy the header into the packet buffer
    memcpy(packet, &header, sizeof(RUDP_Header));

    // Copy the data into the packet buffer after the header
    memcpy(packet + sizeof(RUDP_Header), data, data_length);
    
    // Send the packet over the network using sendto
    int bytes_sent = sendto(sockfd, packet, sizeof(RUDP_Header) + data_length, 0, dest_addr, addrlen);
    
    // Free the memory allocated for the packet buffer
    free(packet);
    
    if (bytes_sent == -1) {
        return -1; // Return error code if sending packet failed
    }
    
    return 0; // Return success status code
}



void rudp_recv(int sockfd, const char *file_path) {
    // Implement RUDP data receiving logic
    // ...
}

void rudp_close(int sockfd) {
    // Close the RUDP connection
    // ...
}
