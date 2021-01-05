#include "client_functions.h"

int main(int argc, char **argv) {
    int sockfd;
    u_int8_t buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    if (argc != 5) {
        fprintf(stderr, "client usage: client hostname portnumber operation key\n");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "client getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket() failed");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect() failed");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "client: could not connect to any\n");
        return 2;
    } else {
        fprintf(stderr, "client: connected\n");
    }

    char * key = argv[4];
    uint16_t key_length = strlen(key);
    uint16_t key_length_network = htons(key_length);
    size_t msg_length = 7 + key_length;
    uint8_t msg[msg_length];
    memset(msg, 0, msg_length);
    *(uint16_t *) (msg + 1) = key_length_network;
    memcpy(msg + 7, key, key_length);

    if (!strncmp("DELETE", argv[3], 6)) {
        fprintf(stderr, "client: operation == DELETE\n");
        msg[0] |= 1 << 0;   // set DELETE bit
        if (send(sockfd, msg, msg_length, 0) == -1){
            perror("client: send() DELETE failed");
        } else {
            fprintf(stderr, "client: DELETE successfully sent\n");
        }

    } else if (!strncmp("SET", argv[3], 3)) {
        fprintf(stderr, "client: operation == SET\n");
        msg[0] |= 1 << 1;   // set SET bit
        int c;
        uint32_t value_length = 0;
        uint8_t * value = NULL;
        while ((c = fgetc(stdin)) != EOF) {
            value_length++;
            value = (uint8_t *) realloc(value, value_length);
            if (value == NULL) {
                fprintf(stderr, "client: failed to allocate memory\n");
                exit(2);
            }
            *(value + value_length - 1) = (uint8_t) c;
//            memcpy(value + value_length - 1, (uint8_t *) &c, 1);
        }
        uint32_t value_length_network = htonl(value_length);
        *(uint32_t *) (msg + 3) = value_length_network;
        size_t msg_complete_length = msg_length + value_length;
        uint8_t * msg_complete = calloc(msg_complete_length, sizeof(uint8_t));
        if (msg_complete == NULL) {
            fprintf(stderr, "client: failed to allocate memory\n");
            exit(3);
        }
        memcpy(msg_complete, msg, msg_length);
        memcpy(msg_complete + msg_length, value, value_length);
        if (send(sockfd, msg_complete, msg_complete_length, 0) == -1){
            perror("client: send() SET failed");
        } else {
            fprintf(stderr, "client: SET successfully sent\n");
        }
        free(value);
        free(msg_complete);

    } else if (!strncmp("GET", argv[3], 3)) {
        fprintf(stderr, "client: operation == GET\n");
        msg[0] |= 1 << 2;   // set GET bit
        if (send(sockfd, msg, msg_length, 0) == -1){
            perror("client: send() GET failed");
        } else {
            fprintf(stderr, "client: GET successfully sent\n");
        }
    } else {
        fprintf(stderr, "client: invalid operation\n");
        exit(4);
    }




    uint64_t numbytes = 7;
    uint8_t * buf_ptr = NULL;
    while (numbytes > 0) {
        buf_ptr = &buf[7 - numbytes];
        uint64_t bytes_received = recv(sockfd, buf_ptr, numbytes, 0);
        if (bytes_received == -1) {
            perror("client: recv() failed in header");
            close(sockfd);
            exit(5);
        }
        numbytes -= bytes_received;
    }
    uint8_t operation = buf[0];
    uint16_t keylen = ntohs(*(uint16_t *) (buf + 1));
    uint32_t valuelen = ntohl(*(uint32_t *) (buf + 3));
    fprintf(stderr, "client:\n\toperation: %u\n\tkeylen: %u\n\tvaluelen: %u\n", operation, keylen, valuelen);

    numbytes = keylen;
    while (numbytes > 0) {
        uint64_t bytes_received;
        if (numbytes < MAXDATASIZE) {
            bytes_received = recv(sockfd, buf, numbytes, 0);
        } else {
            bytes_received = recv(sockfd, buf, MAXDATASIZE, 0);
        }
        if (bytes_received == -1) {
            perror("client: recv() failed");
            exit(6);
        }
        fwrite(buf, sizeof(u_int8_t), bytes_received, stderr);
        numbytes -= bytes_received;
    }

    numbytes = valuelen;
    while (numbytes > 0) {
        uint64_t bytes_received;
        if (numbytes < MAXDATASIZE) {
            bytes_received = recv(sockfd, buf, numbytes, 0);
        } else {
            bytes_received = recv(sockfd, buf, MAXDATASIZE, 0);
        }
        if (bytes_received == -1) {
            perror("client: recv() failed");
            exit(7);
        }
        fwrite(buf, sizeof(u_int8_t), bytes_received, stdout);
        numbytes -= bytes_received;
    }

    close(sockfd);

    return 0;
}
