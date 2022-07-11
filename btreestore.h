#ifndef BTREESTORE_H
#define BTREESTORE_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>

struct info {
    uint32_t size;
    uint32_t key[4];
    uint64_t nonce;
    void * data;
};

struct key_info {
    uint32_t size;
    uint32_t key[4];
    uint64_t nonce;
    void * data;
    void *cache;
};


struct btree {
  struct treenode *root;
  uint16_t branching;
  uint8_t n_processors;
  uint16_t num_nodes;
  pthread_mutex_t lock;
};

struct treenode {
    uint16_t num_keys;
    uint32_t * keys;
    struct key_info **key_info;
    uint16_t num_childs;
    struct treenode **nodes;
    struct treenode *parent;
};

struct node {
    uint16_t num_keys;
    uint32_t * keys;
};

void print_nd(struct treenode *node);

void cleanup(struct treenode *node, struct treenode **list, int *i);

void preorder(struct treenode * node, struct node * list, int * i, void *helper);

void split(struct treenode *node, void * helper);

void * init_store(uint16_t branching, uint8_t n_processors);

void close_store(void * helper);

void decrypt_tea_ctr_HACK(uint64_t * cipher, uint64_t * cache, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks);

struct treenode *create_node(uint32_t *keys, struct key_info **info_blocks, int num_keys, void *helper, struct treenode *parent);

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void * helper);

struct treenode * find_k(uint32_t key, void * helper, int find_dup);

int btree_retrieve(uint32_t key, struct info * found, void * helper);

int btree_decrypt(uint32_t key, void * output, void * helper);

void encrypt_tea_ctr_HACK(uint64_t * plain, uint64_t * cache, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks);

int btree_delete(uint32_t key, void * helper);

uint64_t btree_export(void * helper, struct node ** list);

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]);

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]);

void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks);

void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks);

#endif
