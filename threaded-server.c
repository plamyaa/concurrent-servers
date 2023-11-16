#include "utils.h"

#define PORT "9090"
#define BUFFER_SIZE 1024

typedef struct { int sockfd; } thread_config_t;

typedef enum { WAIT_FOR_MSG, IN_MSG } processing_state;

void serve_connection(int32_t sockfd)
{
    uint8_t buf[BUFFER_SIZE];
    int32_t len, i;

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
    int32_t sockfd = config->sockfd;
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
    thread_config_t *config;
    pthread_t the_thread;
    int32_t sockfd = listen_socket(PORT);

    while(1) {
        int32_t new_fd = accept_with_log(sockfd);

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
