#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

#include "uthash.h"

#define BACKLOG 10

#define MAXDATASIZE 128 // not smaller than 7

#ifndef BLOCK3_HT_H
#define BLOCK3_HT_H

struct my_struct {
    uint8_t * key;              // TODO: should be const ??
    uint8_t * value;
    uint32_t value_length;
    UT_hash_handle hh;          // makes this structure hashable
};
struct my_struct *database;


void hash_add(uint8_t * key, uint16_t key_length, uint8_t * value, uint32_t value_length);

struct my_struct * hash_find(const uint8_t *find_key, int find_key_length);

void hash_delete(struct my_struct * to_be_deleted);

void handle_set(uint8_t * key, uint16_t key_length, uint8_t * value, uint32_t value_length, int fd);

void handle_get(uint8_t * key, uint16_t key_length, int fd);

void handle_delete(uint8_t * key, uint16_t key_length, int fd);


int digits_only(const char *s);

#endif //BLOCK3_HT_H
