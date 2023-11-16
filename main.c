#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT "9090"
#define BACKLOG 20
#define BUFFER_SIZE 1024

typedef struct { int sockfd; } thread_config_t;

typedef enum { WAIT_FOR_MSG, IN_MSG } processing_state;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)&sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)&sa)->sin6_addr);
}

void serve_connection(int sockfd)
{
    uint8_t buf[BUFFER_SIZE];
    int len, i;

    if ((send(sockfd, "*", 1, 0)) == -1) {
        perror("send");
        exit(3);
    }
    
    processing_state state = WAIT_FOR_MSG;

    while(1) {
        if ((len = recv(sockfd, buf, sizeof buf, 0)) == -1) {
            perror("recv");
            exit(4);
        }
        else if (len == 0) {
            break;
        }
        
        for (i = 0; i < len; i++) {
            switch(state) {
                case WAIT_FOR_MSG:
                    if (buf[i] == '^')
                        state = IN_MSG;
                    break;
                case IN_MSG:
                    if (buf[i] == '$') {
                        state = WAIT_FOR_MSG;
                    }

                    else {
                        buf[i] += 1;
                        if ((send(sockfd, &buf[i], 1, 0)) == -1) {
                            perror("send error");
                            close(sockfd);
                            return;
                        }
                    }
                    break;
            }
        }
    }
}

void* server_thread(void *fd)
{
    thread_config_t *config = (thread_config_t *)fd;
    int sockfd = config->sockfd;
    free(config);

    unsigned long id = (unsigned long )pthread_self();
    printf("Thread %lu created to handle connection with socket %d\n", id,
           sockfd);
    serve_connection(sockfd);
    printf("Thread %lu done\n", id);
    return 0;
}

int main(int argc, char* argv[])
{
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_size;
    struct addrinfo hints, *servinfo, *p;
    int status, sockfd, new_fd;
    char s[INET6_ADDRSTRLEN];
    thread_config_t *config;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE; // fill in my IP

    if ((status = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, 
                             p->ai_socktype, 
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                        sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connection...\n");

    while(1) {
        peer_addr_size = sizeof peer_addr;
        if ((new_fd = accept(sockfd, 
                             (struct sockaddr *)&peer_addr, 
                             &peer_addr_size)) == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(peer_addr.ss_family,
                  get_in_addr((struct sockaddr *)&peer_addr), 
                  s, sizeof s);    
        printf("server: got connection from %s\n", s);
        
        pthread_t the_thread;

        if (!(config = (thread_config_t*)malloc(sizeof(*config)))) {
            perror("config");
            exit(2);
        }
        config->sockfd = new_fd;

        if (pthread_create(&the_thread, NULL, server_thread, config) != 0) {
            perror("pthread_create");
            exit(2);
        }

        pthread_detach(the_thread);    
    }

    return 0;
}
