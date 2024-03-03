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

void calculate_and_print_statistics(time_t start_time, time_t end_time, size_t file_size_bytes) {
    double elapsed_time = difftime(end_time, start_time);
    double bandwidth = file_size_bytes / elapsed_time;
    printf("Time taken: %.2f seconds\n", elapsed_time);
    printf("Average bandwidth: %.2f bytes/second\n", bandwidth);
}

int main(int argc, char *argv[]) {
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-algo") != 0) {
        printf("Usage: %s -p <port> -algo <congestion_algorithm>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[2]);
    // Set TCP congestion control algorithm
    const char *congestion_algorithm = argv[4];
    int enable_reuse = 1;
    int listening_socket = -1;

    signal(SIGPIPE, SIG_IGN);

    if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Could not create listening socket");
        return -1;
    }

    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int)) < 0) {
        perror("setsockopt() failed");
        close(listening_socket);
        return -1;
    }

    
    if (strcmp(congestion_algorithm, "reno") == 0) {
        setsockopt(listening_socket, IPPROTO_TCP, algorithm, "reno", strlen("reno"));
    } else if (strcmp(congestion_algorithm, "cubic") == 0) {
        setsockopt(listening_socket, IPPROTO_TCP, congestion_algorithm, "cubic", strlen("cubic"));
    } else {
        printf("Invalid congestion control algorithm specified.\n");
        close(listening_socket);
        return -1;
    }

    // Bind the socket to a specific port
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(listening_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        close(listening_socket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(listening_socket, 1) == -1) {
        perror("listen() failed");
        close(listening_socket);
        return -1;
    }

    // Accept a connection from the sender
    printf("Waiting for incoming TCP connections...\n");

    // Receive the file, measure the time, and save it
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while (1) {
        memset(&client_address, 0, sizeof(client_address));
        client_address_len = sizeof(client_address);
        int client_socket = accept(listening_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("accept() failed");
            close(listening_socket);
            return -1;
        }

        printf("A new client connection accepted\n");

        // Receive file, measure time, and save it
        time_t start_time, end_time;
        time(&start_time); // Start time measurement

        FILE *received_file = fopen("received_file.txt", "wb");
        if (received_file == NULL) {
            perror("Error opening file for writing");
            close(client_socket);
            close(listening_socket);
            return -1;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        size_t total_bytes_received = 0;

        while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            fwrite(buffer, 1, bytes_read, received_file);
            total_bytes_received += bytes_read;
        }

        if (bytes_read < 0) {
            perror("Error receiving file");
            fclose(received_file);
            close(client_socket);
            close(listening_socket);
            return -1;
        }

        fclose(received_file);
        time(&end_time); // End time measurement

        printf("File received successfully.\n");

        // Calculate and print statistics
        calculate_and_print_statistics(start_time, end_time, total_bytes_received);

        // Close client socket
        close(client_socket);
    }

    // Close listening socket
    close(listening_socket);

    return 0;
}
