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

#define BUFFER_SIZE 2097152 // 2 * 1024 * 1024 bytes

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

    if (listen(listening_socket, 1) == -1) {
        perror("listen() failed");
        close(listening_socket);
        return -1;
    }

    printf("Waiting for incoming TCP connections...\n");

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

        printf("Received connection from %s\n", inet_ntoa(client_address.sin_addr));

        time_t start_time, end_time;
        time(&start_time);

        FILE *received_file = fopen("received_file.bin", "wb");
        if (received_file == NULL) {
            perror("Error opening file for writing");
            close(client_socket);
            close(listening_socket);
            return -1;
        }

        char *buffer = (char *)malloc(size);
        if (buffer == NULL) {
            printf("Error allocating memory for buffer\n");
            fclose(file);
            return -1;
        }
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
        time(&end_time);

        printf("Received %zu bytes of data from %s\n", total_bytes_received, inet_ntoa(client_address.sin_addr));

        printf("Waiting for client response...\n");

        char decision;
        if (recv(client_socket, &decision, sizeof(decision), 0) <= 0) {
            perror("Error receiving client response");
            close(client_socket);
            close(listening_socket);
            return -1;
        }

        if (decision == 'y') {
            printf("Client responded with 'y', sending sync byte...\n");
            send(client_socket, "S", 1, 0);
        } else {
            printf("Client responded with 'n'.\n");
        }

        printf("Closing connection...\n");
        close(client_socket);

        calculate_and_print_statistics(start_time, end_time, total_bytes_received);
    }

    close(listening_socket);
    return 0;
}
