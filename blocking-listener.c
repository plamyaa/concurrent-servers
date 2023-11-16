#include "utils.h"

#define PORT "9090"

int main(int argc, char* argv[])
{
    int32_t recv_buf, newsockfd, sockfd;
    uint8_t buf[1024];

    sockfd = listen_socket(PORT);
    newsockfd = accept_with_log(sockfd);

    while (1) {
        printf("Calling recv...\n");
        if ((recv_buf = recv(newsockfd, buf, sizeof buf, 0)) == -1) {
            perror("server: recv");
            exit(2);
        }
        else if (recv_buf == 0) {
            printf("Peer disconnected\n");
            break;
        }
        printf("recv returned %d bytes\n", recv_buf);
    }
    close(newsockfd);
    close(sockfd);
}