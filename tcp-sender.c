// Sender file
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

#define FILE_SIZE 2 * 1000 * 1000
#define BUFFER_SIZE  2 * 1000 // 2 Megabytes buffer size

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

void print_statistics(clock_t start_time, clock_t end_time, size_t file_size_bytes) {
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time taken: %.2f ms\n", elapsed_time);
}

int main(int argc, char *argv[]) {
    if (argc != 7 || strcmp(argv[1], "-ip") != 0 || strcmp(argv[3], "-p") != 0 || strcmp(argv[5], "-algo") != 0) {
        printf("Usage: %s -ip <ip> -p <receiver_port> -algo <congestion_algorithm>\n", argv[0]);
        return -1;
    }

    const char *receiver_ip = argv[2];
    int receiver_port = atoi(argv[4]);
    const char *congestion_algorithm = argv[6];
    const char *file_name = "random_file.bin";

    // Generate random file of at least 2MB size
    size_t file_size_bytes = FILE_SIZE;
    generate_random_file(file_name, file_size_bytes);

    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error creating socket");
        return -1;
    }

    // Set up connection parameters
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(receiver_port);
    inet_pton(AF_INET, receiver_ip, &server_address.sin_addr);

    // Connect to the receiver
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to receiver");
        close(sock);
        return -1;
    }

    // Handshake with the receiver
    char handshake = 'H';
    ssize_t bytes_sent = send(sock, &handshake, sizeof(handshake), 0);
    if (bytes_sent == -1) {
        perror("Error sending handshake");
        close(sock);
        return -1;
    }

    char ack;
    ssize_t bytes_received = recv(sock, &ack, sizeof(ack), 0);
    if (bytes_received <= 0) {
        perror("Error receiving acknowledgment from receiver");
        close(sock);
        return -1;
    }

    // Check if the acknowledgment is valid
    if (ack != 'A') {
        printf("Invalid acknowledgment received from receiver\n");
        close(sock);
        return -1;
    }

    printf("Handshake successful\n");
    printf("Connected to %s:%d\n", receiver_ip, receiver_port);

    char decision = 'n';
    int continue_sending = 1;
    while (1) {

        if (decision == 'y')
            printf("Continuing . . .\n");

        printf("Sending %ld bytes of data . . .\n", FILE_SIZE);
        // Send the file
        FILE *file = fopen(file_name, "rb");
        if (file == NULL) {
            perror("Error opening file for reading");
            close(sock);
            return -1;
        }

        char *buffer = (char *)malloc(BUFFER_SIZE);
        if (buffer == NULL) {
            printf("Failed to create buffer \n");
            fclose(file);
            close(sock);
            return -1;
        }

        clock_t start_time = clock();
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            ssize_t bytes_sent = send(sock, buffer, bytes_read, 0);
            if (bytes_sent == -1) {
                perror("Error sending file");
                fclose(file);
                close(sock);
                return -1;
            }
        }

        clock_t end_time = clock(); 

        fclose(file);
        printf("Successfully sent %ld bytes of data!\n", file_size_bytes);

        // Calculate and print statistics
        print_statistics(start_time, end_time, file_size_bytes);

        // Ask user if they want to send more data
        printf("Do you want to send more data? (y/n): ");
        scanf(" %c", &decision);

        while (decision != 'y' && decision != 'n') {
            printf("Invalid answer, do you want to send more data? (y/n) ");
            scanf(" %c", &decision);
        }

        // Send the decision to the receiver
        ssize_t bytes_sent = send(sock, &decision, sizeof(decision), 0);
        if (bytes_sent == -1) {
            perror("Error sending decision");
            close(sock);
            return -1;
        }

        if (decision == 'n')
            break;
        else if (decision == 'y'){
           printf("Waiting for the server to be ready . . .\n");

            // Receive the sync byte from the receiver
            char sync_byte;
            ssize_t bytes_received = recv(sock, &sync_byte, sizeof(sync_byte), 0);
            if (bytes_received <= 0) {
                perror("Error receiving sync byte from receiver");
                close(sock);
                return -1;
            }

            // Check if the received byte is the sync byte
            if (sync_byte == 'S') {
                printf("Server accepted your decision.\n");
            } else {
                printf("Error: Unexpected response from server.\n");
                close(sock);
                return -1;
            }
        }

    }

    // Close the TCP connection
    close(sock);
    printf("Exited program.\n");

    return 0;
}
