// RUDP_Receiver.c

#include "rdup_api.c"

int main(int argc, char *argv[]) {
    // Argument validation
    // ...

    int port = atoi(argv[2]);

    int sockfd;
    rudp_socket(&sockfd, port);

    // Receive the file using RUDP
    rudp_recv(sockfd, "received_file.txt");

    // Wait for Sender response
    // ...

    // Print out times and average bandwidth
    // ...

    // Calculate average time and total average bandwidth
    // ...

    // Close the RUDP connection
    rudp_close(sockfd);

    // Exit
    // ...

    return 0;
}
