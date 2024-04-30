// Receiver file
#include "rudp_api.c"
#include <netinet/tcp.h> 
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>

#define FILE_SIZE 2 * 1024 * 1024 // 2 Megabytes buffer size
#define BUFFER_SIZE 2 * 1024  // Size of each packet
#define STATISTICS_SIZE 100 // Maximum iterations (resend the file)

void print_statistics(double *timeTaken, double *transferSpeed, int iteration) {
    double total_Time =0;
    double total_Speed=0;
    for(int i=0; i<iteration; i++){
        total_Time+=timeTaken[i];
        total_Speed+=transferSpeed[i];
    }

    double average_Time = total_Time/(iteration);
    double average_Speed = total_Speed/(iteration);
    
    printf("- - - - - - - - - - - - - - - - - -\n");
    printf("\n-        * Statistics *           -\n");
    printf("- The file was sent %d times\n", (iteration));
    printf("- Average time taken to receive the file: %.2f ms.\n", average_Time);
    printf("- Average throughput: %.2f Mbps\n", average_Speed );
    printf("- Total time: %.2f ms\n", total_Time );
    printf("\nIndividual samples:\n");

    for(int j=0; j<iteration; j++){
        printf("- Run #%d Data: Time=%.2f ms; Speed=%.2f Mbps\n", j+1, timeTaken[j], transferSpeed[j]);
    }

    printf("- - - - - - - - - - - - - - - - - -\n");
    printf("Receiver end.\n");

}

void updateStatistics(double *timeTaken, double *transferSpeed, time_t start_time, time_t end_time, size_t total_bytes_received, int iteration) {
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    double throughput_mbps = (total_bytes_received) / (1024 * 1024 * elapsed_time); // Transfer speed in Mbps

    // Calculate time taken in milliseconds
    double time_taken_ms = (elapsed_time * 1000);

    // Insert values into the arrays
    timeTaken[iteration] = time_taken_ms; 
    transferSpeed[iteration] = throughput_mbps; 
}




int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        printf("Usage: %s -p <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[2]);

    printf("Waiting for incoming RUDP connections...\n");

    // Create RUDP socket
    int sockfd;
    struct sockaddr_in *sender_addr = malloc(sizeof(struct sockaddr_in));
    if((sockfd = rudp_socket_receiver(port, sender_addr)) == -1){
        printf("Error creating socket\n");
        return -1;
    }
    
    socklen_t addrlen = sizeof(*sender_addr);

    
    // Convert the client's IP address from network format to presentation format
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &sender_addr->sin_addr, client_ip, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop() failed");
        close(sockfd);
        return -1;
    }

    printf("Connected to %s:%d\n", client_ip, port);

    char *buffer = (char *)malloc(BUFFER_SIZE);

    if (buffer == NULL) {
        printf("Error allocating memory for buffer\n");
        close(sockfd);
        return -1;
    }

    double *timeTaken = (double *)malloc(BUFFER_SIZE * sizeof(double));

    if (timeTaken == NULL) {
        printf("Error allocating memory for timeTaken\n");
        free(buffer);
        close(sockfd);
        return -1;
    }

    double *transferSpeed = (double *)malloc(BUFFER_SIZE * sizeof(double));

    if (transferSpeed == NULL) {
        printf("Error allocating memory for transferSpeed\n");
        free(timeTaken);
        free(buffer);
        close(sockfd);
        return -1;
    }

    // Initialize all elements of transferSpeed,timeTaken to 0
    memset(transferSpeed, 0, BUFFER_SIZE * sizeof(double));
    memset(timeTaken, 0, BUFFER_SIZE * sizeof(double));
    time_t start_time, end_time;
    int iteration=0; //Iteration counter

    while (1) {

        printf("Receiving file from client...\n");
        // Receive the file from the sender
        FILE *file = fopen("received_file.bin", "wb");
        if (file == NULL) {
            perror("Error opening file for writing");
            free(timeTaken);
            free(transferSpeed);
            free(buffer);
            close(sockfd);
            return -1;
        }
        
        start_time = clock();

        size_t total_bytes_received = 0;
        uint8_t bytes_received;

        while (total_bytes_received < FILE_SIZE) {
            // Receive the data
            if((bytes_received = rudp_recv(sockfd, sender_addr, (socklen_t *)addrlen, file) == -1)){
                // Deal with errors
                perror("Error receiving file's content from client");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(sockfd);
                return -1;
            // Deal with timeouts
            }else if(bytes_received == ETIMEDOUT){
                printf("Client %s:%d disconnected.\n" , client_ip, port);
                perror("Timeout receiving file's content from client");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(sockfd);
                return -1;
            }else if(bytes_received != 0){
                printf("Unexpected flag received. Closing connection...\n");
                perror("Error receiving file's content from client");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(sockfd);
                return -1;
            }
        
            //Send acknowledgment for the received packet to sender
            if (rudp_send((const uint8_t *)RUDP_ACK, sizeof(uint8_t), RUDP_ACK, sockfd, sender_addr, addrlen)!= 0) {
                perror("Error writing to file");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                fclose(file);
                close(sockfd);
                return -1;
            }
            total_bytes_received += sizeof(buffer);
        }

        //Send acknowledgment for the received file to sender
        if (rudp_send(RUDP_ACK, sizeof(uint8_t), RUDP_ACK, sockfd, sender_addr, addrlen)!= 0) {
            perror("Error writing to file");
            free(timeTaken);
            free(transferSpeed);
            free(buffer);
            fclose(file);
            close(sockfd);
            return -1;
        }

        end_time = clock();

        printf("Received %zu bytes of data from %s:%d.\n", total_bytes_received, client_ip, port);
        fclose(file);

        updateStatistics(timeTaken, transferSpeed , start_time, end_time, total_bytes_received, iteration);
        iteration++;
        
        printf("Waiting for client's decision...\n");

        //Receive client's decision
        int decision_bytes;
        decision_bytes = rudp_recv(sockfd, sender_addr, (socklen_t *)addrlen, file);
        if (decision_bytes == -1) {
            perror("Error receiving client response");
            free(timeTaken);
            free(transferSpeed);
            free(buffer);
            close(sockfd);
            return -1;
        }else if(decision_bytes == ETIMEDOUT){
            printf("Client %s:%d disconnected.\n" , client_ip, port);
            perror("Timeout receiving file's content from client");
            free(timeTaken);
            free(transferSpeed);
            free(buffer);
            close(sockfd);
            return 0;
        }

        //Decision ' y ' received
        if (decision_bytes == 2) {
            printf("Client responded with 'y', sending ACK byte...\n");
            if (rudp_send(RUDP_ACK, sizeof(uint8_t), RUDP_ACK, sockfd, sender_addr, addrlen)!= 0) {
                perror("Error sending sync byte");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(sockfd);
                return -1;
            }
        //Decision ' n ' received
        } else if (decision_bytes == 3) {
            printf("Client responded with 'n', sending ACK byte...\n");
            if (rudp_send(RUDP_ACK, sizeof(uint8_t), RUDP_ACK, sockfd, sender_addr, addrlen)!= 0) {
                perror("Error sending sync byte");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(sockfd);
                return -1;
            }
            printf("Closing connection...\n");
            // Close the RUDP connection
            free(buffer);
            close(sockfd);
            print_statistics(timeTaken,transferSpeed, iteration);
            free(timeTaken);
            free(transferSpeed);
            return 0;
        //Unexpected response
        } else {
            printf("Unexpected response received. Closing connection...\n");
            free(buffer);
            close(sockfd);
            print_statistics(timeTaken,transferSpeed, iteration);
            free(timeTaken);
            free(transferSpeed);
            return -1;
        }

       
    }

    printf("Closing connection...\n");
    // Close the RUDP connection
    free(timeTaken);
    free(transferSpeed);
    free(buffer);
    close(sockfd);
    
    return 0;
}
