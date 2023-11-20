#include "utils.h"

#define BACKLOG 64
#define N_BACKLOG 64

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)&sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)&sa)->sin6_addr);
}

int32_t listen_socket(const char port[])
{
    struct addrinfo hints, *servinfo, *p;
    int32_t status, sockfd;
    int8_t yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE; // fill in my IP

    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
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
            perror("server: setsockopt");
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
        perror("server: listen");
        exit(1);
    }

    printf("server: waiting for connection...\n");

    return sockfd;
}

int32_t accept_with_log(uint32_t sockfd)
{
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_size;
    int32_t new_fd;
    char s[INET6_ADDRSTRLEN];

    peer_addr_size = sizeof peer_addr;
    if ((new_fd = accept(sockfd, 
                         (struct sockaddr *)&peer_addr, 
                         &peer_addr_size)) == -1) {
        perror("server: accept");
    }

    inet_ntop(peer_addr.ss_family,
              get_in_addr((struct sockaddr *)&peer_addr), 
              s, sizeof s);    
    printf("server: got connection from %s\n", s);

    return new_fd;
}

void set_non_blocking (uint32_t sockfd) 
{
    int32_t flags;
    
    if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1) {
        perror("fcntl F_GETFL");
        exit(2);
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL NONBLOCK");
        exit(2);
    }
}

void* xmalloc(size_t size)
{
    void* ptr = malloc(size);
    
    if (!ptr) {
        fprintf(stderr, "malloc failed");
        exit(1);
    }

    return ptr;
}

void report_peer_connected(const struct sockaddr_in* sa, socklen_t salen)
{
    char hostbuf[NI_MAXHOST];
    char portbuf[NI_MAXSERV];

    if (getnameinfo((struct sockaddr*)sa, salen, hostbuf, NI_MAXHOST, portbuf,
                    NI_MAXSERV, 0) == 0) {
        printf("peer (%s, %s) connected\n", hostbuf, portbuf);
    } 
    else {
        printf("peer (unknonwn) connected\n");
    }
}
