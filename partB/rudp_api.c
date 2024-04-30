#include "rudp_api.h"
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

// Function to generate a random byte
uint8_t generate_random_byte() {
    return (uint8_t)rand() % 256;
}

//Close the connection
void rudp_close(int sockfd) {
    close(sockfd);
}


// Function to send data over RUDP connection with custom header
int rudp_send(const uint8_t *data, size_t data_length, uint8_t flag, int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    // Construct the RUDP header
    RUDP_Header header;
    header.length = data_length; // Set the length field
    header.flag = flag; // Set the flag field
    
    // Calculate checksum for the data (you need to implement this function)
    header.checksum = calculate_checksum(data, data_length);
    
    // Allocate memory for the packet buffer
    uint8_t *packet = (uint8_t *)malloc(Header_Size + data_length);
    if (packet == NULL) {
        perror("Failed to allocate memory for packet buffer\n");
        return -1; // Return error code if memory allocation failed
    }
    
    // Copy the header into the packet buffer
    // Serialize the header into the packet buffer
    memcpy(packet, &(header.length), sizeof(uint16_t)); // Copy length
    memcpy(packet + sizeof(uint16_t), &(header.checksum), sizeof(uint16_t)); // Copy checksum
    memcpy(packet + 2*sizeof(uint16_t), &(header.flag), sizeof(uint8_t)); // Copy flag

    // Copy the data into the packet buffer after the header
    memcpy(packet + sizeof(RUDP_Header), data, data_length);
    
    // Send the packet over the network using sendto
    int bytes_sent = sendto(sockfd, packet, Header_Size + data_length, 0, (struct sockaddr *)dest_addr, addrlen);
    
    // Free the memory allocated for the packet buffer
    free(packet);
    
    if (bytes_sent == -1) {
        perror("Failed to send packet\n");
        return -1; // Return error code if sending packet failed
    }
    
    return 0; // Return success status code
}


// Function to receive data over RUDP connection with custom header
int rudp_recv(size_t packet_length, int sockfd, struct sockaddr_in *src_addr, socklen_t *addrlen, FILE *file) {
    // Allocate memory for the packet buffer
    uint8_t *packet = (uint8_t *)malloc(Header_Size + packet_length);
    if (packet == NULL) {
        return -1; // Return error code if memory allocation failed
    }

    // Check if the source address is NULL
    if (src_addr == NULL) {
        perror("src_addr is NULL\n");
    }

    // Receive the packet over the network using recvfrom
    int bytes_received = recvfrom(sockfd, packet, Header_Size + packet_length, 0, (struct sockaddr *)src_addr, addrlen);
    if (bytes_received == -1) {
        perror("recvfrom");
        free(packet);
        return -1; // Return error code if receiving packet failed
    }

    if(bytes_received == ETIMEDOUT){
        printf("Timeout\n");
        return ETIMEDOUT;
    }

    // Extract header fields from the received packet
    RUDP_Header header;
    memcpy(&header, packet, Header_Size);

    // Calculate data length
    size_t data_length = packet_length - Header_Size;

    // Calculate checksum for the received data (without header)
    uint8_t *data = packet + Header_Size;
    uint16_t checksum = calculate_checksum(data, data_length);

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
    switch (header.flag) {
        case RUDP_ACK:
            // Handle ACK packet
            printf("Received ACK packet\n");
            free(packet);
            return 1;
            break;
        case RUDP_SYN:
            // Handle SYN packet
            printf("Received SYN packet\n");
            free(packet);
            return 2;
            break;
        case RUDP_FIN:
            // Handle FIN packet
            printf("Received FIN packet\n");
            free(packet);
            return 3;
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
             free(packet);

            return 0; // Return success status code for DATA-flag packet
        default:
            // Invalid flag value
            printf("Error: Invalid flag value\n");
            free(packet);
            return -1; // Return error code
    }
}

// Function to set up an RUDP socket for the receiver (server) side and perform handshake
// Gets empty addr and fills in the details of the sender.
int rudp_socket_receiver(int port, struct sockaddr_in *sender_addr) {
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

    //Set timeout for the socket
    struct timeval timeout;      
    timeout.tv_sec = 100;
    timeout.tv_usec = 0;
    
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0){
        perror("setsockopt failed\n");
        close(sockfd);
        return -1;
    }
    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0){
        perror("setsockopt failed\n");
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
        rudp_close(sockfd);
        return -1;
    }

    // Perform handshake receive and obtain sender's address
    socklen_t addrlen = sizeof(*sender_addr);

    // Generate a random byte for the handshake message
    uint8_t handshake_byte = generate_random_byte();

    // Receive the handshake-SYN message using RUDP
    int errorcode = rudp_recv(sizeof(uint8_t),sockfd, sender_addr, &addrlen, NULL);
    if(errorcode == -1){
        perror("Error receiving handshake message\n");
        rudp_close(sockfd);
        return -1;
    }
    else if (errorcode != 2) {
        perror("Unexpected flag on handshake message received. Expected SYN.");
        rudp_close(sockfd);
        return -1;
    }

    printf("Handshake SYN message received.\n");
    printf("Sending handshake ACK message.\n");

    // Send the handshake message using RUDP
    if (rudp_send(&handshake_byte, sizeof(handshake_byte), RUDP_ACK, sockfd, sender_addr, addrlen) == -1) {
        perror("Error sending handshake message\n");
        rudp_close(sockfd);
        return -1;
    }

    printf("Handshake completed successfully.\n");

    // Return the socket descriptor
    return sockfd;
}



// Function to set up an RUDP socket for the sender (client) side and perform handshake
// Gets empty addr and fills in the details of the receiver.

int rudp_socket_sender(const char *dest_ip, int dest_port, struct sockaddr_in *receiver_addr) {

    // Create a RUDP socket
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

    //Set timeout for the socket
    struct timeval timeout;      
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;
    
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                sizeof timeout) < 0)
        perror("setsockopt failed\n");

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                sizeof timeout) < 0)
        perror("setsockopt failed\n");

    // Set up the receiver address structure
    memset(receiver_addr, 0, sizeof(*receiver_addr));
    receiver_addr->sin_family = AF_INET;
    receiver_addr->sin_port = htons(dest_port);
    int errorcode = inet_pton(AF_INET, dest_ip, &receiver_addr->sin_addr);
    if (errorcode == -1) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    } else if(errorcode == 0) {
        fprintf(stderr, "Invalid IP address\n");
        close(sockfd);
        return -1;
    }

    // Generate a random byte for the handshake message
    uint8_t handshake_byte = generate_random_byte();

    printf("Sending handshake SYN message.\n");
    printf("Handshake byte: %u\n", handshake_byte);
    // Send the handshake message using RUDP
    if (rudp_send(&handshake_byte, sizeof(handshake_byte), RUDP_SYN, sockfd, receiver_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("Error sending handshake message\n");
        close(sockfd);
        return -1;
    }

    // Receive the handshake-ACK message using RUDP
    errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, (socklen_t *)sizeof(receiver_addr), NULL);
    
    int retries = 0;

    // Expecting an ACK flag
    while(errorcode!=1 && retries < MAX_RETRIES){

        if(errorcode == -1){
        perror("Error receiving handshake message\n");
        close(sockfd);
        return -1;
        
        //Unexpected flag received - SYN or FIN
        }else if (errorcode == 2 || errorcode == 3) {
            perror("Unexpected flag on handshake message received. Expected SYN.");
            close(sockfd);
            return -1;
        }

        // Resend the handshake message using RUDP - deal with timeout
        if (rudp_send(&handshake_byte, sizeof(handshake_byte), RUDP_SYN, sockfd, receiver_addr, sizeof(receiver_addr)) == -1) {
            perror("Error sending handshake message\n");
            close(sockfd);
            return -1;
        }
        // Receive the handshake-ACK message using RUDP
        int addrln = sizeof(receiver_addr);
        errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, (socklen_t *)&addrln, NULL);
        retries++;
    }
    

    printf("Handshake SYN message received.\n");
    printf("Handshake completed successfully.\n");

    // Return the socket descriptor
    return sockfd;
}



