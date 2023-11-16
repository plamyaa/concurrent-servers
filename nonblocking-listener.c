#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

#define PORT "9090"

int main(int argc, char* argv[])
{
    int32_t sockfd, newsockfd, flags, recv_buf;
    uint8_t buf[1024];
    
    sockfd = listen_socket(PORT);
    newsockfd = accept_with_log(sockfd);

    if ((flags = fcntl(newsockfd, F_GETFL, 0)) == -1) {
        perror("fcntl F_GETFL");
        exit(2);
    }

    if (fcntl(newsockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL NONBLOCK");
        exit(2);
    }

    while (1) {
        printf("Calling recv...\n");
        if ((recv_buf = recv(newsockfd, buf, sizeof buf, 0)) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(200 * 1000);
            }
            perror("server: recv");
            continue;
        }
        else if (recv_buf == 0) {
            printf("Peer disconnected\n");
            break;
        }
        printf("recv returned %d bytes\n", recv_buf);
    }

    close(newsockfd);
    close(sockfd);

    return 0;
}