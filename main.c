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


#define PORT "8080"
#define BACKLOG 20

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)&sa)->sin_addr);

    return &(((struct sockaddr_in6 *)&sa)->sin6_addr);
}

int main(int argc, char* argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *servinfo, *p;
    int status, sockfd, new_fd;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE; // fill in my IP

    if ((status = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = servinfo; p != NULL; p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, 
                             p->ai_socktype, 
                             p->ai_protocol)) == -1) 
        {
            perror("server: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connection...\n");

    while(1)
    {   
        addr_size = sizeof their_addr;
        if ((new_fd = accept(sockfd, 
                             (struct sockaddr *)&their_addr, 
                            &their_addr)) == -1)    
        {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr), 
                  s, sizeof s);    

        if (send(new_fd, "Hello", 5, 0) == -1)
            perror("send");
        close(new_fd);
    }

    return 0;
}
