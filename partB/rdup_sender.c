// Sender file
#include <rdup_api.c>
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

    // Generate random file of at least 2MB size
    size_t file_size_bytes = FILE_SIZE;
    generate_random_file(file_name, file_size_bytes);

    int sockfd;
    struct sockaddr_in *receiver_addr = malloc(sizeof(struct sockaddr_in));
    if((sockfd = rudp_socket(receiver_ip, receiver_port, receiver_addr)) == -1){
        printf("Error creating socket\n");
        return -1;
    }
    

    char decision = 'n';
    char *buffer = (char *)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
            printf("Failed to create buffer \n");
            free(buffer);
            close(sockfd);
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
            close(sockfd);
            return -1;
        }

        clock_t start_time = clock();

        size_t total_bytes_received = 0;
        size_t bytes_received;

        //Extract data from file
        while ((bytes_received = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {

            // Send the data
            if(rudp_send(bytes_received, buffer , RUDP_DATA, sockfd, receiver_addr, sizeof(receiver_addr)) == -1){
                perror("Error sending file");
                free(buffer);
                close(sockfd);
                return -1;
            }

            // Receive ACK for current packet
            int errorcode = rudp_recv(sockfd, receiver_addr, sizeof(receiver_addr), file);

            // Resend data if timeout + handle errors
            while(errorcode!=0){

                //Error receiving ACK
                if(errorcode == -1){
                    perror("Error receiving ACK");
                    free(buffer);
                    close(sockfd);
                    return -1;
                }

                //Timeout handling - send data again
                rudp_send((uint8_t)decision, sizeof(uint8_t) , RUDP_SYN, sockfd, receiver_addr, sizeof(receiver_addr));

                // Receive ACK for current packet
                errorcode = rudp_recv(sockfd, receiver_addr, sizeof(receiver_addr), file);
            }
            
            total_bytes_received += bytes_received;
        }

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
            if(rudp_send((uint8_t)decision, sizeof(uint8_t) , RUDP_FIN, sockfd, receiver_addr, sizeof(receiver_addr)) == -1){
                perror("Error sending decision");
                free(buffer);
                close(sockfd);
                return -1;
            }

            // Receive ACK for decision
            int errorcode = rudp_recv(sockfd, receiver_addr, sizeof(receiver_addr), file);

            // Resend data if timeout + handle errors
            while(errorcode!=0){

                //Error receiving ACK
                if(errorcode == -1){
                    perror("Error receiving ACK");
                    free(buffer);
                    close(sockfd);
                    return -1;
                }

                //Timeout handling - send data again
                rudp_send((uint8_t)decision, sizeof(uint8_t) , RUDP_SYN, sockfd, receiver_addr, sizeof(receiver_addr));

                // Receive ACK for current packet
                errorcode = rudp_recv(sockfd, receiver_addr, sizeof(receiver_addr), file);
            }
            break;
        }
        else if (decision == 'y'){

            // Send the decision to the receiver
            if(rudp_send((uint8_t)decision, sizeof(uint8_t) , RUDP_SYN, sockfd, receiver_addr, sizeof(receiver_addr)) == -1){
                perror("Error sending decision");
                free(buffer);
                close(sockfd);
                return -1;
            }

            printf("Waiting for the server to be ready...\n");

            // Receive ACK for decision
            int errorcode = rudp_recv(sockfd, receiver_addr, sizeof(receiver_addr), file);

            // Resend data if timeout + handle errors
            while(errorcode!=0){

                //Error receiving ACK
                if(errorcode == -1){
                    perror("Error receiving ACK");
                    free(buffer);
                    close(sockfd);
                    return -1;
                }

                //Timeout handling - send data again
                rudp_send((uint8_t)decision, sizeof(uint8_t) , RUDP_SYN, sockfd, receiver_addr, sizeof(receiver_addr));

                // Receive ACK for current packet
                errorcode = rudp_recv(sockfd, receiver_addr, sizeof(receiver_addr), file);
            }
    
            printf("Server accepted your decision.\n");
        }

    }

    printf("Closing connection...\n");
    // Close the RUDP connection
    free(buffer);
    close(sockfd);

    return 0;
}
