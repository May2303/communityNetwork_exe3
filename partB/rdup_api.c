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

//Flags:
#define RUDP_DATA 0x00 // Standard data transfering packet flag
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

// Function to set up an RUDP socket for the receiver (server) side and perform handshake
int rudp_socket(int port, struct sockaddr_in *sender_addr) {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    // Set the SO_REUSEADDR option to allow address reuse
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    // Set up the sender address structure for receiving handshake
    memset(sender_addr, 0, sizeof(*sender_addr));
    sender_addr->sin_family = AF_INET;
    sender_addr->sin_addr.s_addr = INADDR_ANY;
    sender_addr->sin_port = htons(port);

    // Bind the socket to the local address
    if (bind(sockfd, (struct sockaddr *)sender_addr, sizeof(*sender_addr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // Perform handshake receive and obtain sender's address
    socklen_t addrlen = sizeof(*sender_addr);
    if (receive_handshake(sockfd, (struct sockaddr *)sender_addr, &addrlen) == -1) {
        perror("receive_handshake");
        close(sockfd);
        return -1;
    }

    // Perform handshake send to send acknowledgment
    if (send_handshake(sockfd, (struct sockaddr *)sender_addr, addrlen) == -1) {
        perror("send_handshake");
        close(sockfd);
        return -1;
    }

    printf("Handshake completed successfully.\n");

    // Return the socket descriptor
    return sockfd;
}



// Function to set up an RUDP socket for the sender (client) side and perform handshake
int rudp_socket(const char *dest_ip, int dest_port, struct sockaddr_in *receiver_addr) {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    // Set the SO_REUSEADDR option to allow address reuse
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    // Set up the receiver address structure
    memset(receiver_addr, 0, sizeof(*receiver_addr));
    receiver_addr->sin_family = AF_INET;
    receiver_addr->sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &receiver_addr->sin_addr) != 1) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    // Send handshake message to the receiver
    if (send_handshake(sockfd, (struct sockaddr *)receiver_addr, sizeof(*receiver_addr)) == -1) {
        perror("send_handshake");
        close(sockfd);
        return -1;
    }

    // Receive handshake acknowledgment from the receiver
    socklen_t addrlen = sizeof(*receiver_addr);
    if (receive_handshake(sockfd, (struct sockaddr *)receiver_addr, &addrlen) == -1) {
        perror("receive_handshake");
        close(sockfd);
        return -1;
    }

    printf("Handshake completed successfully.\n");

    // Return the socket descriptor
    return sockfd;
}



// Function to send data over RUDP connection with custom header
int rudp_send(const uint8_t *data, size_t data_length, uint8_t flag, int sockfd, struct sockaddr *dest_addr, socklen_t addrlen) {
    // Construct the RUDP header
    RUDP_Header header;
    header.length = data_length; // Set the length field
    header.flag = flag; // Set the flag field
    
    // Calculate checksum for the data (you need to implement this function)
    header.checksum = calculate_checksum(data, data_length);
    
    // Allocate memory for the packet buffer
    uint8_t *packet = (uint8_t *)malloc(sizeof(RUDP_Header) + data_length);
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


//TODO: Deal with each flag case.
// Function to receive data over RUDP connection with custom header
int rudp_recv(int sockfd, struct sockaddr *src_addr, socklen_t *addrlen, FILE *file) {
    // Allocate memory for the packet buffer
    uint8_t *packet = (uint8_t *)malloc(sizeof(RUDP_Header) + PACKET_SIZE);
    if (packet == NULL) {
        return -1; // Return error code if memory allocation failed
    }

    // If src_addr is NULL, allocate memory for it and set addrlen accordingly
    struct sockaddr_in sender_addr; // Temporary variable to hold sender's address
    if (src_addr == NULL) {
        src_addr = (struct sockaddr *)&sender_addr;
        *addrlen = sizeof(sender_addr);
    }

    // Receive the packet over the network using recvfrom
    int bytes_received = recvfrom(sockfd, packet, sizeof(RUDP_Header) + PACKET_SIZE, 0, src_addr, addrlen);
    if (bytes_received == -1) {
        perror("recvfrom");
        free(packet);
        return -1; // Return error code if receiving packet failed
    }

    // Extract header fields from the received packet
    RUDP_Header header;
    memcpy(&header, packet, sizeof(RUDP_Header));

    // Calculate data length
    int data_length = bytes_received - sizeof(RUDP_Header);

    // Calculate checksum for the received data (without header)
    uint16_t checksum = calculate_checksum(packet + sizeof(RUDP_Header), data_length);

    // Compare checksum with the checksum field in the header
    if (checksum != header.checksum) {
        printf("Checksum mismatch: header checksum = %d, actual checksum = %d\n", header.checksum, checksum);
        free(packet);
        return -1; // Return error code if checksum verification failed
    }

    // Compare length with the length field in the header
    if (header.length != data_length) {
        printf("Length mismatch: header length = %d, actual length = %d\n", header.length, data_length);
        free(packet);
        return -1; // Return error code if length mismatch
    }

    // Handle the received packet based on the flag value
    switch (header.flags) {
        case RUDP_ACK:
            // Handle ACK packet
            printf("Received ACK packet\n");
            break;
        case RUDP_SYN:
            // Handle SYN packet
            printf("Received SYN packet\n");
            break;
        case RUDP_FIN:
            // Handle FIN packet
            printf("Received FIN packet\n");
            break;
        case RUDP_DATA:
            // Open the file in "append binary" mode
            file = fopen("received_data.bin", "ab");
            if (file == NULL) {
                perror("fopen");
                free(packet);
                return -1; // Return error code if file opening failed
            }

            // Write the received data to the file
            fwrite(packet + sizeof(RUDP_Header), sizeof(uint8_t), data_length, file);
            if (file != NULL) {
                fclose(file);
            }
            break;
        default:
            // Invalid flag value
            printf("Error: Invalid flag value\n");
            free(packet);
            return -1; // Return error code
    }

    free(packet);

    return 0; // Return success status code
}


void rudp_close(int sockfd) {
    // Close the RUDP connection
    // ...
}

// Function to generate a random byte
uint8_t generate_random_byte() {
    return (uint8_t)rand() % 256;
}


// Function to send handshake message
int send_handshake(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen) {
    // Generate a random byte for the handshake message
    uint8_t handshake_byte = generate_random_byte();

    // Send the handshake message using RUDP
    if (rudp_send(&handshake_byte, sizeof(handshake_byte), RUDP_ACK, sockfd, dest_addr, addrlen) == -1) {
        perror("rudp_send");
        return -1;
    }

    printf("Handshake message sent.\n", handshake_byte);

    
    // Handshake message successfuly sent
    return 0;
}


// Function to receive handshake message
int receive_handshake(int sockfd, struct sockaddr *sender_addr, socklen_t *addrlen) {

   // Receive the handshake acknowledgment using RUDP
    if (rudp_recv(sockfd, dest_addr, &addrlen, NULL) == -1) {
        perror("rudp_recv");
        return -1;
    }
    printf("Handshake message received.\n");

    // Handshake message successfuly received
    return 0;
}
