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

#define BUFFER_SIZE 1024
#define FILE_SIZE_MB 2

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

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <receiver_ip> <receiver_port> <congestion_algorithm>\n", argv[0]);
        return -1;
    }

    const char *receiver_ip = argv[1];
    int receiver_port = atoi(argv[2]);
      // Set TCP congestion control algorithm
    const char *congestion_algorithm = argv[3];
    const char *file_name = "random_file.bin";

    // Generate random file of at least 2MB size
    size_t file_size_bytes = FILE_SIZE_MB * 1024 * 1024 * 8; //Megabyte is 8m
    generate_random_file(file_name, file_size_bytes);

    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error creating socket");
        return -1;
    }

  
    if (strcmp(congestion_algorithm, "reno") == 0) {
        setsockopt(sock, IPPROTO_TCP, congestion_algorithm, "reno", strlen("reno"));
    } else if (strcmp(congestion_algorithm, "cubic") == 0) {
        setsockopt(sock, IPPROTO_TCP, congestion_algorithm, "cubic", strlen("cubic"));
    } else {
        printf("Invalid congestion control algorithm specified.\n");
        close(sock);
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

    while (1) {
        // Send the file
        FILE *file = fopen(file_name, "rb");
        if (file == NULL) {
            perror("Error opening file for reading");
            close(sock);
            return -1;
        }

        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            ssize_t bytes_sent = send(sock, buffer, bytes_read, 0);
            if (bytes_sent == -1) {
                perror("Error sending file");
                fclose(file);
                close(sock);
                return -1;
            }
        }

        // Close the current file
        fclose(file);

        // User decision: Send the file again or exit
        printf("Do you want to send another file? (y/n): ");
        char decision;
        scanf(" %c", &decision);

        if (decision != 'y' && decision != 'Y') {
            // If the user enters something other than 'y' or 'Y', exit the loop
            break;
        }
    }

    // Close the TCP connection
    close(sock);

    return 0;
}
