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

#define BUFFER_SIZE 2 * 1024 * 1024 * 8 // 2 Megabytes buffer size

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
    if (argc != 7 || strcmp(argv[1], "-ip") != 0 || strcmp(argv[3], "-port") != 0 || strcmp(argv[5], "-algo") != 0) {
        printf("Usage: %s -ip <ip> -port <receiver_port> -algo <congestion_algorithm>\n", argv[0]);
        return -1;
    }

    const char *receiver_ip = argv[2];
    int receiver_port = atoi(argv[4]);
    const char *congestion_algorithm = argv[6];
    const char *file_name = "random_file.bin";

    // Generate random file of at least 2MB size
    size_t file_size_bytes = BUFFER_SIZE;
    generate_random_file(file_name, file_size_bytes);

    

    char decision='n';
    while (1) {
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
        
        if(decision == 'y')
            printf("Continuing . . .\n");


        printf("Sending %ld bytes of data . . .\n", BUFFER_SIZE);
        // Send the file
        clock_t start_time = clock();
        FILE *file = fopen(file_name, "rb");
        if (file == NULL) {
            perror("Error opening file for reading");
            close(sock);
            return -1;
        }

        char *buffer = (char *)malloc(BUFFER_SIZE);
        if(buffer == NULL){
            printf("Failed to create buffer \n");
            free(buffer);
            return -1;
        }
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

        fclose(file);
        clock_t end_time = clock();

        printf("Successfully sent %ld bytes of data!\n", file_size_bytes);

        // Calculate and print statistics
        print_statistics(start_time, end_time, file_size_bytes);

        // Ask user if they want to send more data
        printf("Do you want to send more data? (y/n): ");
        scanf(" %c", &decision);

        if (decision != 'y' && decision != 'n') {
            printf("Invalid answer, do you want to send more data? (y/n) ");
            scanf(" %c", &decision);
        }

        if (decision == 'n')
            break;
        if(decision == 'y')
            printf("Waiting for the server to be ready . . .\n");

        // Close the TCP connection
        close(sock);
    }


    printf("Exited program.\n");
    

    return 0;
}
