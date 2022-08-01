#include "peer_functions.h"


int main(int argc, char **argv) {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (argc != 2) {
        fprintf(stderr, "peer usage: peer port\n");
        exit(1);
    }
    if (!digits_only(argv[1])) {
        fprintf(stderr,"peer: wrong input! only digits for port allowed!\n");
        exit(2);
    }
//    int port = atoi(argv[1]);


    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "peer getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("peer: socket() failed");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("peer: setsockopt() failed");
            exit(3);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("peer: bind() failed");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "peer: could not bind to any\n");
        exit(4);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("peer: listen() failed");
        exit(5);
    }

    fprintf(stderr, "peer: waiting for connections...\n");

    while (free) {
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("peer: accept() failed");
            close(new_fd);
//            continue;
            close(sockfd);
            exit(6);
        }
        fprintf(stderr, "peer: got connection\n");

        uint8_t buf[MAXDATASIZE];
        uint64_t numbytes = 7;
        uint8_t * buf_ptr = NULL;
        while (numbytes > 0) {
            buf_ptr = &buf[7 - numbytes];
            uint64_t bytes_received = recv(new_fd, buf_ptr, numbytes, 0);
            if (bytes_received == -1) {
                perror("peer: recv() failed in header");
                close(new_fd);
                close(sockfd);
                exit(7);
            }
            numbytes -= bytes_received;
        }
        uint8_t operation = buf[0];
        uint16_t keylen = ntohs(*(uint16_t *) (buf + 1));
        uint32_t valuelen = ntohl(*(uint32_t *) (buf + 3));
        fprintf(stderr, "peer:\n\toperation: %u\n\tkeylen: %u\n\tvaluelen: %u\n", operation, keylen, valuelen);

        numbytes = keylen + valuelen;
        uint8_t * dynamic_buffer = NULL;
        uint64_t bytes_total = 0;
        while (bytes_total < numbytes) {
            uint64_t bytes_received;
            if (numbytes < MAXDATASIZE) {
                bytes_received = recv(new_fd, buf, numbytes, 0);
            } else {
                bytes_received = recv(new_fd, buf, MAXDATASIZE, 0);
            }
            if (bytes_received == -1) {
                perror("peer: recv() failed after header");
                close(new_fd);
                close(sockfd);
                free(dynamic_buffer);
                exit(8);
            }
            bytes_total += bytes_received;
            dynamic_buffer = (uint8_t *) realloc(dynamic_buffer, bytes_total);
            if (dynamic_buffer == NULL) {
                fprintf(stderr, "peer: failed to allocate memory\n");
                exit(9);
            }
            memcpy(dynamic_buffer + bytes_total - bytes_received, buf, bytes_received);
        }
        uint8_t * key = (uint8_t *) calloc(bytes_total - valuelen, sizeof(uint8_t));
        uint8_t * value = (uint8_t *) calloc(bytes_total - keylen, sizeof(uint8_t));
        if (key == NULL || value == NULL) {
            fprintf(stderr, "peer: failed to allocate memory\n");
            exit(10);
        }
        memcpy(key, dynamic_buffer, bytes_total - valuelen);
        memcpy(value, dynamic_buffer + keylen, bytes_total - keylen);
        free(dynamic_buffer);

        fprintf(stderr, "peer: recv() done\n");

        fprintf(stderr, "peer: key == \"");
        fwrite(key, sizeof(uint8_t), keylen, stderr);
        fprintf(stderr, "\"\n");

        if (operation == 2) {           // SET
            handle_set(key, keylen, value, valuelen, new_fd);
        } else if (operation == 4) {    // GET
            handle_get(key, keylen, new_fd);
            free(key);
            free(value);
        } else if (operation == 1) {    // DELETE
            handle_delete(key, keylen, new_fd);
            free(key);
            free(value);
        }

        close(new_fd);
    }
    close(sockfd);

    return 0;
}