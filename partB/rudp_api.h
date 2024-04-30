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

//Flags:
#define RUDP_DATA 0x00 // Standard data transfering packet flag
#define RUDP_SYN 0x01 // Sync flag
#define RUDP_ACK 0x02 // Acknowledgement flag
#define RUDP_FIN 0x04 // Ending program/connection flag

#define Header_Size 5 // Size of the header
#define PACKET_SIZE 1024 // Packet size
#define MAX_RETRIES 5 // Maximum number of retries for sending a packet
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
uint16_t calculate_checksum(const uint8_t *data, int packet_length);

// Function to generate a random byte
uint8_t generate_random_byte();

//Close the connection
void rudp_close(int sockfd);

// Function to send data over RUDP connection with custom header
int rudp_send(const uint8_t *data, size_t data_length, uint8_t flag, int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen);

// Function to receive data over RUDP connection with custom header
int rudp_recv(size_t packet_length,int sockfd, struct sockaddr_in *src_addr, socklen_t *addrlen, FILE *file);

// Function to set up an RUDP socket for the receiver (server) side and perform handshake
// Gets empty addr and fills in the details of the sender.
int rudp_socket_receiver(int port, struct sockaddr_in *sender_addr);

// Function to set up an RUDP socket for the sender (client) side and perform handshake
// Gets empty addr and fills in the details of the receiver.

int rudp_socket_sender(const char *dest_ip, int dest_port, struct sockaddr_in *receiver_addr);