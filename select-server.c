#include "utils.h"

#include <assert.h>
#include <stdbool.h>
#include <sys/select.h>

#define PORT "9090"
#define MAXFDS 1000
#define SENDBUF_SIZE 1024

typedef enum { INITIAL_ACK, WAIT_FOR_MSG, IN_MSG } processing_state;

typedef struct {
    bool want_read;
    bool want_write;
} fd_status_t;

typedef struct {
    processing_state state;
    uint8_t sendbuf[SENDBUF_SIZE];
    int32_t sendbuf_end;
    int32_t sendptr;
} peer_state_t;

peer_state_t global_state[MAXFDS];

const fd_status_t fd_status_R = { .want_read = true, .want_write = false };
const fd_status_t fd_status_W = { .want_read = false, .want_write = true };
const fd_status_t fd_status_RW = { .want_read = true, .want_write = true };
const fd_status_t fd_status_NORW = { .want_read = false, .want_write = false };

fd_status_t on_peer_connected(int32_t sockfd) {
    assert(sockfd < MAXFDS);

    peer_state_t* peerstate = &global_state[sockfd];
    peerstate->state = INITIAL_ACK;
    peerstate->sendbuf[0] = '*';
    peerstate->sendbuf_end = 1;
    peerstate->sendptr = 0;

    return fd_status_R;
}

fd_status_t on_peer_ready_recv(int32_t sockfd) {
    int32_t n_bytes;
    uint8_t buf[1024];
    bool ready_to_send = false;
    assert(sockfd < MAXFDS);

    peer_state_t* peerstate = &global_state[sockfd];

    if (peerstate->state == INITIAL_ACK || 
        peerstate->sendptr < peerstate->sendbuf_end) {
        return fd_status_W;
    }

    if ((n_bytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return fd_status_R;
        }
        else {
            perror("server: recv");
            exit(2);
        }
    }
    else if (n_bytes == 0) {
        return fd_status_NORW;
    }

    for (uint32_t i = 0; i < n_bytes; i++) {
        switch(peerstate->state) {
            case INITIAL_ACK:
                assert(0 && "Can't reach here");
                break;
            case WAIT_FOR_MSG:
                if (buf[i] == '^')
                    peerstate->state = IN_MSG;
                break;

            case IN_MSG:
                if (buf[i] == '$') {
                    peerstate->state = WAIT_FOR_MSG;
                }

                else {
                    assert(peerstate->sendbuf_end < SENDBUF_SIZE);
                    peerstate->sendbuf[peerstate->sendbuf_end++] = buf[i] + 1;
                    ready_to_send = true;
                }
                break;
        }
    }

    return (fd_status_t) {.want_read = !ready_to_send,
                          .want_write = ready_to_send};
}

fd_status_t on_peer_ready_send(int32_t sockfd) {
    int32_t sendlen, n_send;
    assert(sockfd < MAXFDS);

    peer_state_t* peerstate = &global_state[sockfd];

    if (peerstate->sendptr >= peerstate->sendbuf_end) { // empty
        return fd_status_RW;
    }

    sendlen = peerstate->sendbuf_end - peerstate->sendptr;
    if ((n_send = send(sockfd, &peerstate->sendbuf[peerstate->sendptr], sendlen,
                      0)) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return fd_status_W; 
        }
        else {
            perror("server: send");
            exit(2);
        }
    }

    if (n_send < sendlen) {
        peerstate->sendptr += n_send;
        return fd_status_W;
    }
    else {
        peerstate->sendptr = 0;
        peerstate->sendbuf_end = 0;
    
        if (peerstate->state == INITIAL_ACK) {
            peerstate->state = WAIT_FOR_MSG;
        }

        return fd_status_R;
    }
}

int main(int argc, char* argv[]) 
{
    int32_t sockfd, n_ready, newsockfd, listener_sockfd;

    listener_sockfd = listen_socket(PORT);

    set_non_blocking(listener_sockfd);

    if (listener_sockfd >= FD_SETSIZE) {
        printf("listener socket fd (%d) >= FD_SETSIZE (%d)", listener_sockfd,
               FD_SETSIZE);
        exit(2);
    }

    fd_set readfds_master;
    FD_ZERO(&readfds_master);
    fd_set writefds_master;
    FD_ZERO(&writefds_master);

    FD_SET(listener_sockfd, &readfds_master);

    int32_t fdset_max = listener_sockfd;

    while (1) {
        fd_set readfds = readfds_master;
        fd_set writefds = writefds_master;

        if ((n_ready = select(fdset_max + 1, &readfds, &writefds, NULL, 
                        NULL)) == -1) {
            perror("server: select");
            exit(2);
        }

        for (int32_t fd = 0; fd <= fdset_max && n_ready > 0; fd++) {
            if (FD_ISSET(fd, &readfds)) {
                n_ready--;

                if (fd == listener_sockfd) { // new peer connected
                    if ((newsockfd = accept_with_log(fd)) == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            printf("accept returned EAGAIN or EWOULDBLOCK\n");
                        }
                        else {
                            perror("server: accept");
                            exit(2);
                        }
                    }
                    else {
                        set_non_blocking(newsockfd);
                        if (newsockfd > fdset_max) {
                            if (newsockfd >= FD_SETSIZE) {
                                printf("socket fd (%d) >= FD_SETSIZE (%d)", 
                                      newsockfd, FD_SETSIZE);
                                exit(2);
                            }
                            fdset_max = newsockfd;
                        }

                        fd_status_t status = on_peer_connected(newsockfd);
                        if (status.want_read) {
                            FD_SET(newsockfd, &readfds_master);
                        } else {
                            FD_CLR(newsockfd, &readfds_master);
                        }
                        if (status.want_write) {
                            FD_SET(newsockfd, &writefds_master);
                        } else {
                            FD_CLR(newsockfd, &writefds_master);
                        }
                    }
                }
                else {
                    printf("handle recv");
                    fd_status_t status = on_peer_ready_recv(fd);

                    if (status.want_read) {
                        FD_SET(fd, &readfds_master);
                    } else {
                        FD_CLR(fd, &readfds_master);
                    }
                    if (status.want_write) {
                        FD_SET(fd, &writefds_master);
                    } else {
                        FD_CLR(fd, &writefds_master);
                    }

                    if (!status.want_read && !status.want_write) {
                        printf("socket %d closing\n", fd);
                        close(fd);
                    }
                }
            }
            if (FD_ISSET(fd, &writefds)) {
                n_ready--;
                printf("send");
                fd_status_t status = on_peer_ready_send(fd);

                if (status.want_read) {
                    FD_SET(fd, &readfds_master);
                } else {
                    FD_CLR(fd, &readfds_master);
                }
                if (status.want_write) {
                    FD_SET(fd, &writefds_master);
                } else {
                    FD_CLR(fd, &writefds_master);
                }

                if (!status.want_read && !status.want_write) {
                    printf("socket %d closing\n", fd);
                    close(fd);
                }
            }
        }
    }

    return 0;
}