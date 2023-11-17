#pragma once

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
#include <fcntl.h>

void *get_in_addr(struct sockaddr *sa);

int32_t listen_socket(const char port[]);

int32_t accept_with_log(uint32_t sockfd);

void set_non_blocking (uint32_t sockfd);
