// Receiver file
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

void print_statistics(double *timeTaken, double *transferSpeed, const char *algo, int iteration) {
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
    printf("- Algorithm: %s\n", algo);
    printf("- The file was sent %d times\n", (iteration));
    printf("- Average time taken to receive the file: %.2f ms.\n", average_Time);
    printf("- Average throughput: %.2f Mbps\n", average_Speed );
    printf("- Total time: %.2f ms\n", total_Time );
    printf("\nIndividual samples:\n");

    for(int j=0; j<iteration; j++){
        printf("- Run #%d Data: Time=%.2f ms; Speed=%.2f Mbps\n", j+1, timeTaken[j], transferSpeed[j]);
    }

    printf("- - - - - - - - - - - - - - - - - -\n");
    printf("Receiver end.");

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

    // Set TCP congestion control algorithm
    if (strcmp(congestion_algorithm, "cubic") != 0 && strcmp(congestion_algorithm, "reno") != 0 ) {
        printf("Invalid congestion algorithm: %s\n", congestion_algorithm);
        close(sock);
        return -1;
    }

    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, congestion_algorithm, strlen(algorithm)) < 0) {
        perror("setsockopt() failed");
        close(sock);
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
    int client_socket;

    // Accept the connection from the sender
    memset(&client_address, 0, sizeof(client_address));
    client_address_len = sizeof(client_address);

    client_socket = accept(listening_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (client_socket == -1) {
        perror("accept() failed");
        close(listening_socket);
        return -1;
    }

    // Retrieve the client's IP address using getpeername
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    if (getpeername(client_socket, (struct sockaddr *)&peer_addr, &peer_addr_len) == -1) {
        perror("getpeername() failed");
        close(client_socket);
        close(listening_socket);
        return -1;
    }

    // Convert the client's IP address from network format to presentation format
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &peer_addr.sin_addr, client_ip, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop() failed");
        close(client_socket);
        close(listening_socket);
        return -1;
    }

    // Send acknowledgment back to sender
    char ack = 'A';
    if (send(client_socket, &ack, sizeof(ack), 0) == -1) {
        perror("Error sending acknowledgment to sender");
        close(client_socket);
        close(listening_socket);
        return -1;
    }

    // Wait for acknowledgment from sender
    int receivedData = recv(client_socket, &ack, sizeof(ack), 0);
    if (receivedData < 0) {
        perror("Error receiving acknowledgment from sender");
        close(client_socket);
        close(listening_socket);
        return -1;
    }else if(receivedData ==0){
        printf("Client %s:%d disconnected.\n" , client_ip, port);
        close(client_socket);
        close(listening_socket);
        return 0;
    }

    // Check if the acknowledgment is valid
    if (ack != 'H') {
        printf("Invalid acknowledgment received from receiver\n");
        close(client_socket);
        return -1;
    }

    printf("Handshake successful - connection established.\n");

    printf("Connected to %s:%d\n", client_ip, port);

    char *buffer = (char *)malloc(BUFFER_SIZE);

    if (buffer == NULL) {
        printf("Error allocating memory for buffer\n");
        close(client_socket);
        close(listening_socket);
        return -1;
    }

    double *timeTaken = (double *)malloc(BUFFER_SIZE * sizeof(double));

    if (timeTaken == NULL) {
        printf("Error allocating memory for timeTaken\n");
        free(buffer);
        close(client_socket);
        close(listening_socket);
        return -1;
    }

    double *transferSpeed = (double *)malloc(BUFFER_SIZE * sizeof(double));

    if (transferSpeed == NULL) {
        printf("Error allocating memory for transferSpeed\n");
        free(timeTaken);
        free(buffer);
        close(client_socket);
        close(listening_socket);
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
            close(client_socket);
            close(listening_socket);
            return -1;
        }
        
        start_time = clock();

        size_t total_bytes_received = 0;
        size_t bytes_received;

        while (total_bytes_received < FILE_SIZE) {
            if((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) <= 0){
                perror("Error receiving file's content from client");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(client_socket);
                close(listening_socket);
                return -1;
            }else if(bytes_received ==0){
                printf("Client %s:%d disconnected.\n" , client_ip, port);
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(client_socket);
                close(listening_socket);
                return 0;
            }
                 
            size_t bytes_written = fwrite(buffer, 1, bytes_received, file);
            if (bytes_written != bytes_received) {
                perror("Error writing to file");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                fclose(file);
                close(client_socket);
                close(listening_socket);
                return -1;
            }
            total_bytes_received += bytes_received;
        }

        end_time = clock();

        printf("Received %zu bytes of data from %s:%d.\n", total_bytes_received, client_ip, port);
        fclose(file);

        updateStatistics(timeTaken, transferSpeed , start_time, end_time, total_bytes_received, iteration);
        iteration++;
        
        printf("Waiting for client's decision...\n");

        char decision;
        int decision_bytes;
        decision_bytes = recv(client_socket, &decision, sizeof(decision), 0);
        if (decision_bytes < 0) {
            perror("Error receiving client response");
            free(timeTaken);
            free(transferSpeed);
            free(buffer);
            close(client_socket);
            close(listening_socket);
            return -1;
        }else if(decision_bytes == 0){
            printf("Client %s:%d disconnected.\n" , client_ip, port);
            free(timeTaken);
            free(transferSpeed);
            free(buffer);
            close(client_socket);
            close(listening_socket);
            return 0;
        }

        if (decision == 'y') {
            printf("Client responded with 'y', sending sync byte...\n");
            if (send(client_socket, "S", 1, 0) <= 0) {
                perror("Error sending sync byte");
                free(timeTaken);
                free(transferSpeed);
                free(buffer);
                close(client_socket);
                close(listening_socket);
                return -1;
            }
        } else if (decision == 'n') {
            printf("Client responded with 'n'.\n");
            printf("Closing connection...\n");
            // Close the TCP connection
            free(buffer);
            close(client_socket);
            close(listening_socket);
            print_statistics(timeTaken,transferSpeed,congestion_algorithm, iteration);
            free(timeTaken);
            free(transferSpeed);
            return 0;
        } else {
            printf("Unexpected response received. Closing connection...\n");
            return -1;
        }

       
    }

    printf("Closing connection...\n");
    // Close the TCP connection
    free(timeTaken);
    free(transferSpeed);
    free(buffer);
    close(client_socket);
    close(listening_socket);
    
    return 0;
}
