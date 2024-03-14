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

// Function to set up a UDP receiver socket
int set_udp_receiver_socket(int port) {
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

    // Set up the local address structure
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port);

    // Bind the socket to the local address
    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // Return the socket descriptor
    return sockfd;
}

// Function to set up a UDP socket for the sender (client) side
int set_sender_socket(const char *dest_ip, int dest_port) {
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

    // Set up the destination address structure
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) != 1) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    // Return the socket descriptor
    return sockfd;
}

// Function to send data over RUDP connection with custom header
int rudp_send(const uint8_t *data, uint8_t flag, int sockfd, struct sockaddr *dest_addr, socklen_t addrlen) {
    // Construct the RUDP header
    RUDP_Header header;
    header.length = PACKET_SIZE; // Set the length field
    header.flag = flag; // Set the flag field
    
    // Calculate checksum for the data (you need to implement this function)
    header.checksum = calculate_checksum(data, PACKET_SIZE);
    
    // Allocate memory for the packet buffer
    uint8_t *packet = malloc(sizeof(RUDP_Header) + PACKET_SIZE);
    if (packet == NULL) {
        return -1; // Return error code if memory allocation failed
    }
    
    // Copy the header into the packet buffer
    memcpy(packet, &header, sizeof(RUDP_Header));

    // Copy the data into the packet buffer after the header
    memcpy(packet + sizeof(RUDP_Header), data, PACKET_SIZE);
    
    // Send the packet over the network using sendto
    int bytes_sent = sendto(sockfd, packet, sizeof(RUDP_Header) + PACKET_SIZE, 0, dest_addr, addrlen);
    
    // Free the memory allocated for the packet buffer
    free(packet);
    
    if (bytes_sent == -1) {
        return -1; // Return error code if sending packet failed
    }
    
    return 0; // Return success status code
}


//TODO: Go over this function and complete it. It is uncompleted.
// Function to receive data over RUDP connection with custom header
int rudp_recv(int sockfd, struct sockaddr *src_addr, socklen_t *addrlen, FILE *file) {

    uint8_t *buffer = (uint8_t *)malloc(PACKET_SIZE); //Buffer to read data received

    // Receive the packet over the network using recvfrom
    int bytes_received = recvfrom(sockfd, buffer, sizeof(RUDP_Header)+ PACKET_SIZE, 0, src_addr, addrlen);
    if (bytes_received == -1) {
        perror("recvfrom");
        return -1; // Return error code if receiving packet failed
    }

    // Extract header fields from the received packet
    RUDP_Header header;
    memcpy(&header, buffer, sizeof(RUDP_Header));

    // Extract data from the received packet
    uint8_t *data = buffer + sizeof(RUDP_Header);
    int data_length = bytes_received - sizeof(RUDP_Header);

    // Calculate checksum for the received data
    uint16_t checksum = calculate_checksum(data, data_length);

    // Compare checksum with the checksum field in the header
    if (checksum != header.checksum) {
        printf("Checksum verification failed\n");
        return -1; // Return error code if checksum verification failed
    }

    // Write the received data to the file
    fwrite(data, sizeof(uint8_t), data_length, file);

    return 0; // Return success status code
}

void rudp_close(int sockfd) {
    // Close the RUDP connection
    // ...
}

/ Function to generate a random byte
uint8_t generate_random_byte() {
    return (uint8_t)rand() % 256;
}


// TODO: Fix sum check + rudp_recv call
// Function to perform handshake with the receiver
int sender_handshake(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen) {
    // Generate a random byte for the handshake message
    uint8_t handshake_byte = generate_random_byte();

    // Send the handshake message using RUDP
    if (rudp_send(&handshake_byte, sizeof(handshake_byte), RUDP_ACK, sockfd, dest_addr, addrlen) == -1) {
        perror("rudp_send");
        return -1;
    }

    printf("Handshake message sent: %u\n", handshake_byte);

    // Receive the handshake acknowledgment using RUDP
    uint8_t recv_byte;
    if (rudp_recv(&recv_byte, sizeof(recv_byte), sockfd, dest_addr, &addrlen) == -1) {
        perror("rudp_recv");
        return -1;
    }

    printf("Handshake acknowledgment received: %u\n", recv_byte);

    // Calculate checksum for the received byte
    uint16_t checksum = calculate_checksum(&recv_byte, sizeof(recv_byte));

    // Compare checksum with the acknowledgment packet's checksum
    if (checksum != 0) {
        printf("Checksum verification failed\n");
        return -1;
    }

    printf("Handshake successful\n");

    // Handshake successful
    return 0;
}

// TODO: Fix sum check + rudp_recv call
// Function to perform handshake with the sender
int receiver_handshake(int sockfd, struct sockaddr *sender_addr, socklen_t *addrlen) {
    // Receive the handshake message using RUDP
    uint8_t recv_byte;
    if (rudp_recv(&recv_byte, sizeof(recv_byte), sockfd, sender_addr, addrlen) == -1) {
        perror("rudp_recv");
        return -1;
    }

    printf("Handshake message received: %u\n", recv_byte);

    // Calculate checksum for the received byte
    uint16_t checksum = calculate_checksum(&recv_byte, sizeof(recv_byte));

    // Compare checksum with the handshake message's checksum
    if (checksum != 0) {
        printf("Checksum verification failed\n");
        return -1;
    }

    // Generate a random byte for the handshake acknowledgment
    uint8_t ack_byte = generate_random_byte();

    // Send the handshake acknowledgment using RUDP
    if (rudp_send(&ack_byte, sizeof(ack_byte), RUDP_ACK, sockfd, sender_addr, *addrlen) == -1) {
        perror("rudp_send");
        return -1;
    }

    printf("Handshake acknowledgment sent: %u\n", ack_byte);

    // Handshake successful
    return 0;
}
