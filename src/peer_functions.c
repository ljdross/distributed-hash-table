#include "../lib/peer_functions.h"


void hash_add(uint8_t * key, uint16_t key_length, uint8_t * value, uint32_t value_length) {
    fprintf(stderr, "peer: adding new entry to hash table...\n");
    struct my_struct * new_entry = NULL;
    new_entry = calloc(1, sizeof(*new_entry));
    if (new_entry == NULL) {
        fprintf(stderr, "peer: failed to allocate memory\n");
        exit(11);
    }
    new_entry->key = key;
    new_entry->value = value;
    new_entry->value_length = value_length;
    HASH_ADD_KEYPTR(hh, database, new_entry->key, key_length, new_entry);
    fprintf(stderr, "peer: done!\n");
}

struct my_struct * hash_find(const uint8_t *find_key, int find_key_length) {
    fprintf(stderr, "peer: trying to find key...\n");
    struct my_struct * found_entry = NULL;
    HASH_FIND(hh, database, find_key, find_key_length, found_entry);
    if (found_entry){
        fprintf(stderr, "peer: found!\n");
    } else {
        fprintf(stderr, "peer: not found!\n");
    }
    return found_entry;
}

void hash_delete(struct my_struct * to_be_deleted) {
    fprintf(stderr, "peer: trying to delete entry...\n");
    HASH_DELETE(hh, database, to_be_deleted);
    free(to_be_deleted->key);
    free(to_be_deleted->value);
    free(to_be_deleted);
    fprintf(stderr, "done!\n");
}

void handle_set(uint8_t * key, uint16_t key_length, uint8_t * value, uint32_t value_length, int fd) {
    struct my_struct * entry = NULL;
    entry = hash_find(key, key_length);
    if (entry) {
        fprintf(stderr, "peer: need to delete existing entry first\n");
        hash_delete(entry);
    }
    hash_add(key, key_length, value, value_length);
    uint8_t msg[7] = {0, 0, 0, 0, 0, 0, 0};
    msg[0] |= 1 << 1;
    msg[0] |= 1 << 3;
    if (send(fd, msg, 7, 0) == -1) {
        perror("peer: send() ACK SET failed");
    } else {
        fprintf(stderr, "peer: ACK SET, successfully sent\n");
    }
}

void handle_get(uint8_t * key, uint16_t key_length, int fd) {
    struct my_struct * entry = NULL;
    entry = hash_find(key, key_length);
    if (!entry) {
        fprintf(stderr, "peer: GET failed because item was not found\n");
        uint8_t msg[7] = {0, 0, 0, 0, 0, 0, 0};
        msg[0] |= 1 << 2;   // set GET bit
        if (send(fd, msg, 7, 0) == -1) {
            perror("peer: send() non-ACK GET failed");
        } else {
            fprintf(stderr, "peer: non-ACK GET successfully sent\n");
        }
        return;
    }
    uint32_t value_length = entry->value_length;
    uint8_t * msg = NULL;
    msg = (uint8_t *) calloc(7 + key_length + value_length, 1);
    if (msg == NULL) {
        fprintf(stderr, "peer: failed to allocate memory\n");
        exit(12);
    }
    msg[0] |= 1 << 2;   // set GET bit
    msg[0] |= 1 << 3;   // set ACK bit
    uint16_t key_length_network = htons(key_length);
    *(uint16_t *) (msg + 1) = key_length_network;
    uint32_t value_length_network = htonl(value_length);
    *(uint32_t *) (msg + 3) = value_length_network;
    memcpy(msg + 7, key, key_length);
    memcpy(msg + 7 + key_length, entry->value, value_length);
    if (send(fd, msg, 7 + key_length + value_length, 0) == -1){
        perror("peer: send() ACK GET failed");
    } else {
        fprintf(stderr, "peer: ACK GET successfully sent\n");
    }
    free(msg);
}

void handle_delete(uint8_t * key, uint16_t key_length, int fd) {
    uint8_t msg[7] = {0, 0, 0, 0, 0, 0, 0};
    msg[0] |= 1 << 0; // msg[0] = msg[0] | 1; // set DELETE bit
    struct my_struct * entry = NULL;
    entry = hash_find(key, key_length);
    if (!entry) {
        fprintf(stderr, "peer: DELETE failed because item was not found\n");
    } else {
        hash_delete(entry);
        msg[0] |= 1 << 3; // set ACK bit
    }
    if (send(fd, msg, 7, 0) == -1) {
        perror("peer: send() non-ACK/ACK DELETE failed");
    } else {
        fprintf(stderr, "peer: non-ACK/ACK DELETE successfully sent\n");
    }
}

int digits_only(const char *s) {
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }
    return 1;
}
