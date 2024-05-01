// Sender file
#include "rudp_api.h"
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
#include <stdint.h>




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
    if (argc != 5 || strcmp(argv[1], "-ip") != 0 || strcmp(argv[3], "-p") != 0) {
        printf("Usage: %s -ip <ip> -p <receiver_port>\n", argv[0]);
        return -1;
    }

    const char *receiver_ip = argv[2];
    int receiver_port = atoi(argv[4]);
    const char *file_name = "random_file.bin";

    uint8_t syn = RUDP_SYN;
    uint8_t fin = RUDP_FIN;

    // Generate random file of at least 2MB size
    size_t file_size_bytes = FILE_SIZE;
    generate_random_file(file_name, file_size_bytes);

    // Create RUDP socket
    int sockfd;
    struct sockaddr_in *receiver_addr = malloc(sizeof(struct sockaddr_in));
    if((sockfd = rudp_socket_sender(receiver_ip ,receiver_port, receiver_addr)) == -1){
        printf("Error creating socket\n");
        free(receiver_addr);
        return -1;
    }

    socklen_t addrlen = sizeof(*receiver_addr);
    

    char decision = 'n';
    uint8_t *buffer = (uint8_t *)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
            printf("Failed to create buffer \n");
            free(buffer);
            close(sockfd);
            free(receiver_addr);
            return -1;
    }
    while (1) {

        if (decision == 'y')
            printf("Continuing...\n");

        printf("Sending %d bytes of data...\n", FILE_SIZE);
        // Send the file
        FILE *file = fopen(file_name, "rb");
        if (file == NULL) {
            perror("Error opening file for reading");
            free(buffer);
            free(receiver_addr);
            close(sockfd);
            return -1;
        }

        clock_t start_time = clock();

        size_t total_bytes_received = 0;
        size_t bytes_received;

        //Extract data from file
        while (total_bytes_received < FILE_SIZE) {

            // Read data from file
            bytes_received = fread(buffer, 1, BUFFER_SIZE, file);
            if (bytes_received < BUFFER_SIZE) {
                perror("Error reading file");
                free(buffer);
                free(receiver_addr);
                close(sockfd);
                return -1;
            }

            // Send the data to the receiver
            if(rudp_send(buffer, sizeof(uint8_t)*BUFFER_SIZE , RUDP_DATA, sockfd, receiver_addr, addrlen) == -1){
                perror("Error sending file");
                free(buffer);
                free(receiver_addr);
                close(sockfd);
                return -1;
            }

            // Receive ACK for current packet
            int errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, &addrlen, file);

            int retries = 0;
            // If not received ACK proerly
            // Resend data if timeout + handle errors
            while(errorcode!=1 && retries < MAX_RETRIES){

                //Flags for SYN or FIN received
                if(errorcode == 2 || errorcode == 3 ){
                    printf("Unexpected flag received. Closing connection...\n");
                    perror("Error receiving ACK for sent packet");
                    free(buffer);
                    free(receiver_addr);
                    close(sockfd);
                    return -1;
                }

                //Error receiving ACK
                if(errorcode == -1){
                    perror("Error receiving ACK for sent packet");
                    free(buffer);
                    free(receiver_addr);
                    close(sockfd);
                    return -1;
                }

                printf("Timeout detected - resending current packet\n");
                //Timeout handling - send data again
                if(rudp_send(buffer, sizeof(uint8_t)*BUFFER_SIZE , RUDP_DATA, sockfd, receiver_addr, addrlen) == -1){
                perror("Error sending file");
                free(buffer);
                free(receiver_addr);
                close(sockfd);
                return -1;
                }

                // Receive ACK for current resent packet
                errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, &addrlen, file);
                retries++;
            }
            
            total_bytes_received += bytes_received;
        }

        // Receive ACK message after all data has been sent
        if(rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, &addrlen, file) != 1){
            printf("Unexpected flag received. Closing connection...\n");
            perror("Error receiving ACK for sent file");
            free(buffer);
            free(receiver_addr);
            close(sockfd);
            return -1;
        }

        printf("ACK message received - whole file has been sent.\n");
        clock_t end_time = clock(); 

        fclose(file);
        printf("Successfully sent %ld bytes of data!\n", total_bytes_received);

        // Calculate and print statistics
        print_statistics(start_time, end_time);

        // Ask user if they want to send more data
        printf("Do you want to send more data? (y/n): ");
        scanf(" %c", &decision);

        while (decision != 'y' && decision != 'n') {
            printf("Invalid answer, do you want to send more data? (y/n) ");
            scanf(" %c", &decision);
        }

        if (decision == 'n'){
            // Send the decision to the receiver
            if(rudp_send((uint8_t *)&fin, sizeof(uint8_t) , RUDP_FIN, sockfd, receiver_addr, addrlen) == -1){
                perror("Error sending decision");
                free(buffer);
                free(receiver_addr);
                close(sockfd);
                return -1;
            }

            // Receive ACK for decision
            int errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, &addrlen, file);

            

            int retries = 0;
            // If not received ACK proerly
            // Resend data if timeout + handle errors
            while(errorcode!=1 && retries < MAX_RETRIES){

                //Flags for SYN or FIN received
                if(errorcode == 2 || errorcode == 3 ){
                    printf("Unexpected flag received. Closing connection...\n");
                    perror("Error receiving ACK for sent packet");
                    free(buffer);
                    free(receiver_addr);
                    close(sockfd);
                    return -1;
                }

                //Error receiving ACK
                if(errorcode == -1){
                    perror("Error receiving ACK for sent packet");
                    free(buffer);
                    free(receiver_addr);
                    close(sockfd);
                    return -1;
                }
                
                printf("Timeout detected - resending your FIN message\n");
                //Timeout handling - send data again
                if(rudp_send((uint8_t *)&fin, sizeof(uint8_t) , RUDP_FIN, sockfd, receiver_addr, addrlen) == -1){
                perror("Error sending file");
                free(buffer);
                free(receiver_addr);
                close(sockfd);
                return -1;
                }

                // Receive ACK for current resent packet
                errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, &addrlen, file);
                retries++;
            }

            printf("Received ACK message for your FIN message.\n");
            break;
        }
        else if (decision == 'y'){

            // Send the decision to the receiver
            if(rudp_send((uint8_t *)&syn, sizeof(uint8_t) , RUDP_SYN, sockfd, receiver_addr, addrlen) == -1){
                perror("Error sending decision");
                free(buffer);
                free(receiver_addr);
                close(sockfd);
                return -1;
            }

            printf("Waiting for the server to be ready...\n");

            // Receive ACK for decision
            int errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, &addrlen, file);

            int retries = 0;
            // If not received ACK proerly
            // Resend data if timeout + handle errors
            while(errorcode!=1 && retries < MAX_RETRIES){

                //Flags for SYN or FIN received
                if(errorcode == 2 || errorcode == 3 ){
                    printf("Unexpected flag received. Closing connection...\n");
                    perror("Error receiving ACK for sent packet");
                    free(buffer);
                    free(receiver_addr);
                    close(sockfd);
                    return -1;
                }

                //Error receiving ACK
                if(errorcode == -1){
                    perror("Error receiving ACK for sent packet");
                    free(buffer);
                    free(receiver_addr);
                    close(sockfd);
                    return -1;
                }
                
                printf("Timeout detected - resending your SYN message\n");
                //Timeout handling - send data again
                if(rudp_send((uint8_t *)&syn, sizeof(uint8_t) , RUDP_SYN, sockfd, receiver_addr, addrlen) == -1){
                perror("Error sending file");
                free(buffer);
                free(receiver_addr);
                close(sockfd);
                return -1;
                }

                // Receive ACK for current resent packet
                errorcode = rudp_recv(sizeof(uint8_t),sockfd, receiver_addr, &addrlen, file);
                retries++;
            }

            printf("Received ACK message for your SYN message.\n");
            printf("Server accepted your decision.\n");
        }

    }

    printf("Closing connection...\n");
    // Close the RUDP connection
    free(buffer);
    free(receiver_addr);
    close(sockfd);

    return 0;
}
