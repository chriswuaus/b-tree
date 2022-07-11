#include "btreestore.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

// struct btree {
//   struct node *root;
//   uint16_t branching;
//   uint8_t n_processors;
//   uint16_t num_nodes;
// };

// struct treenode{
//   uint16_t num_keys;
//   uint32_t *keys;
// //   struct treenode *nodes;
//     // num_childs
// // // // };
// int main() {
//     // Your own testing code here
//     void * helper = init_store(3, 4);
//     struct btree *btree = helper;
//     char * word = "1234567890123456789555";
//     char *word1 = "hewwols";

//     // btree_insert(2, (void *)NULL, 1, NULL, 1, helper);
//     uint32_t a[4];
//     a[0] = 1;
//     a[1] = 7;
//     a[2] = 5;
//     a[3] = 9;
//     uint32_t b[4];
//     b[0] = 1;
//     b[1] = 9;
//     b[2] = 5;
//     b[3] = 9;

//     // for (int i = 200; i < 726; i ++) {
//     //     btree_insert(i, (void *)word, 22, b, 1234, helper);
//     //     // printf("%d", NULL != find_k(i, helper, 0));
//     // }
//     for (int i = 0; i < 200; i++)
//     {
//         // btree_insert(i, (void *)word, 22, b, 1234, helper);
//         printf("%d", btree_insert(i, (void *)word, 22, b, 1234, helper));
//         printf("\n%d", NULL == find_k(i, helper, 1));
//     }

//     for (int i = 0; i < 200; i++)
//     {
//         // btree_insert(i, (void *)word, 22, b, 1234, helper);
//         // printf("%d", btree_insert(i, (void *)word, 22, b, 1234, helper));
//         printf("\n%d", NULL == find_k(i, helper, 1));
//     }

//     // for (int i = 10200; i < 10727; i ++)
//     // {
//     //     char output[22];
//     //     printf("%d", btree_decrypt(i, &output, helper));
//     // }

//     // for (int i = 200; i < 726; i++) {
//     //     char output[22];
//     //     // btree_decrypt(i, &output, helper);
//     //     printf("%d", btree_decrypt(i, &output, helper));
//     // }
//     // btree_insert(4, (void *)word, 7, a, 23445, helper);
//     // btree_insert(20, (void *)word, 7, a, 345, helper);

//     // btree_insert(22, (void *)word, 7, a, 1234, helper);
//     // btree_insert(21, (void *)word, 7, a, 1234, helper);
//     // btree_insert(80, (void *)word, 7, a, 1345, helper);
//     // btree_insert(1, (void *)word, 7, a, 1456, helper);
//     // btree_insert(3, (void *)word, 7, b, 1234, helper);

//     // btree_retrieve(5, found, helper);
//     // printf("%dsdf asdfas\n",   NULL ==find_k(3, helper, 0));

//     int i = 0;
//     // printf("[numnodes]%d\n", btree->num_nodes);
//     struct node ** list = malloc(sizeof(struct node) * btree->num_nodes);
//     btree_export(helper, list);
//     // btree_export(helper, list);

//     // close_store(helper);

//     return 0;
// }

void * init_store(uint16_t branching, uint8_t n_processors) {
    if (branching < 2 || n_processors < 1) {
        return NULL;
    }
    struct btree *helper = malloc(sizeof(struct btree));
    helper->root =  NULL;
    helper->branching = branching;
    helper->n_processors = n_processors;
    helper->num_nodes = 0;
    pthread_mutex_init(&helper->lock, NULL);
    return helper;
}

void close_store(void * helper) {
    if (helper == NULL) {
        return;
    }
    struct btree * btree = helper;
    struct treenode ** addresses = malloc(sizeof(struct treenode *) * btree->num_nodes);
    pthread_mutex_lock(&btree->lock);
    int i = 0;
    cleanup(btree->root, addresses, &i);
    for (int i = 0; i < btree->num_nodes; i++) {
        for (int j = 0; j < addresses[i]->num_keys; j++) {
            free((addresses[i])->key_info[j]->data);
            free((addresses[i])->key_info[j]->cache);
            free((addresses[i])->key_info[j]);
        }
        free((addresses[i])->nodes);
        free((addresses[i])->keys);
        free((addresses[i])->key_info);
        free((addresses[i]));
    }
    free(addresses);
    free(btree);
    pthread_mutex_unlock(&btree->lock);

    return;
}

void cleanup(struct treenode *node, struct treenode **list, int *i) {
    if (node == NULL){
        return;
    }
    list[*i] = node;
    for (int j = 0; j < node->num_childs; j++) {
        *i += 1;
        cleanup(node->nodes[j], list, i);
    }
}

struct treenode * create_node(uint32_t *keys, struct key_info ** info_blocks, int num_keys, void * helper, struct treenode * parent) {
    struct btree *btree = helper;
    struct treenode *node = malloc(sizeof(struct treenode));
    node->num_keys = num_keys;
    node->keys = malloc(sizeof(uint32_t) * (btree->branching));
    node->key_info = malloc(sizeof(struct key_info) * (btree->branching));
    for (int i = 0; i < num_keys; i++) {
        node->keys[i] = keys[i];
        node->key_info[i] = info_blocks[i];
    }
    node->parent = parent;
    node->nodes = malloc(sizeof(struct treenode) * (btree->branching + 1));
    node->num_childs = 0;
    btree->num_nodes++;
    return node;
}

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void * helper) {
    struct btree *btree = helper;

    if ((find_k(key, helper, 1) != NULL)){
        return 1;
    }
    if (plaintext == NULL || count <= 0 || encryption_key == NULL || helper == NULL) {
        return 1;
    }

    struct key_info *info_block = malloc(sizeof(struct key_info));
    int num_blocks = (count / 8) + ((count % 8) != 0); // ceiling divides count by 64
    info_block->size = count;

    info_block->key[0] = encryption_key[0];
    info_block->key[1] = encryption_key[1];
    info_block->key[2] = encryption_key[2];
    info_block->key[3] = encryption_key[3];


    info_block->nonce = nonce;
    info_block->data = malloc(sizeof(uint64_t) * num_blocks);
    info_block->cache = malloc(sizeof(uint64_t) * num_blocks);

    void *copy = calloc(num_blocks, sizeof(uint64_t));
    memcpy(copy, plaintext, count);

    encrypt_tea_ctr_HACK(copy, info_block->cache, encryption_key, nonce, info_block->data, num_blocks);

    free(copy);
    pthread_mutex_lock(&btree->lock);
    struct treenode *found = find_k(key, helper, 0);

    if (btree->root == NULL) {
        btree->root = create_node(&key, &info_block, 1, helper, NULL);
        pthread_mutex_unlock(&btree->lock);
        return 0;
    }
    
    int inserted = 0;
    
    for (int i = 0; i < found->num_keys; i++) {
        if (found->keys[i] >= key) {
            memmove((void *)(&(found->keys[i + 1])), (void *)(&(found->keys[i])), (found->num_keys - i) * sizeof(uint32_t));
            memmove((void *)(&(found->key_info[i + 1])), (void *)(&(found->key_info[i])), (found->num_keys - i) * sizeof(struct key_info));

            found->keys[i] = key;
            found->key_info[i] = info_block;

            inserted = 1;
            break;
        }
    }

    if (!inserted) {
        found->keys[found->num_keys] = key;
        found->key_info[found->num_keys] = info_block;
    }
    found->num_keys++;
    
    if (found->num_keys == (btree->branching)) {
        split(found, helper);
    }

    pthread_mutex_unlock(&btree->lock);
    return 0;
}

void print_nd(struct treenode *node) {
    printf("[keys in this node]");

    for (int i = 0; i < node->num_keys; i++) {
        printf("[num %d]: %d, ", i, node->keys[i]);
    }
    printf("\n");
}

void split(struct treenode *node, void * helper) {

    struct btree *btree = helper;
    
    int median = -1;
    if ((node->num_keys % 2) == 0) {
        median = (node->num_keys / 2) - 1;
    } else {
        median = (node->num_keys - 1) / 2;
    }

    if (node == btree->root) { // node is transformed into new root!
        struct treenode *new_root = create_node(&node->keys[median], &node->key_info[median], 1, helper, NULL);
        new_root->num_childs = 2;
        new_root->nodes[0] = node;
        node->parent = new_root;
        new_root->nodes[1] = create_node(node->keys + median + 1, node->key_info + median + 1, node-> num_keys - median - 1, helper, new_root); // right child node

        new_root->num_keys = 1;
        node->num_keys = median;
        btree->root = new_root;
        if (node->num_childs > 0) {
            int i = 0;
            for (;i < node->num_childs; i++) {
                if (node->nodes[i]->keys[0] > node->keys[median]) {
                    break;
                }
            }
            memcpy((void*)&(new_root->nodes[1]->nodes[0]), (void *)&(node->nodes[i]), (node->num_childs - i) * sizeof(struct treenode *));
            for (int j = 0; j < node->num_childs - i; j++) {
                new_root->nodes[1]->nodes[j]->parent = new_root->nodes[1]; // set parents of right child node
            }
            new_root->nodes[1]->num_childs = node->num_childs - i;
            node->num_childs = i;
        }
        return;
    }

    // move the median into parent
    int inserted = 0;
    int moved = -1;
    struct treenode *parent = node->parent;
    for (int i = 0; i < parent->num_keys; i++) {
        if (parent->keys[i] >= node->keys[median]) {
            memmove((void *)(&(parent->keys[i + 1])), (void *)(&(parent->keys[i])), (parent->num_keys - i) * sizeof(uint32_t));
            memmove((void *)(&(parent->key_info[i + 1])), (void *)(&(parent->key_info[i])), (parent->num_keys - i) * sizeof(struct key_info));

            parent->keys[i] = node->keys[median];
            parent->key_info[i] = node->key_info[median];

            inserted = 1;
            moved = i;
            // print_nd(node);
            break;
        }
    }
    if (!inserted) {
        parent->keys[parent->num_keys] = node->keys[median];
        parent->key_info[parent->num_keys] = node->key_info[median];

        moved = parent->num_keys;      
    }
    parent->num_keys++;
 
    // move over nodes and place in the right position
    memmove((void *)(&(parent->nodes[moved + 2])), (void *)(&(parent->nodes[moved + 1])), (parent->num_childs - moved + 1) * sizeof(struct treenode));
    parent->nodes[moved + 1] = create_node(node->keys + median + 1, node->key_info + median + 1, node-> num_keys - median - 1, helper, parent);
    node->num_keys = median; // restrict the amount of nodes we can access (only the left nodes)
    parent->num_childs++;


    if (node->num_childs > 0) {
        int i = 0;
        for (;i < node->num_childs; i++) {
            if (node->nodes[i]->keys[0] > node->keys[median]) {
                break;
            }
        }
        memmove((void *) &(parent->nodes[moved + 1]->nodes[0]), (void *)&(node->nodes[i]), (node->num_childs - i) * sizeof(struct treenode));
        parent->nodes[moved + 1]->num_childs = node->num_childs - i;
        node->num_childs = i;
    }


    for (int i = 0; i < parent->nodes[moved + 1]->num_childs; i++) {
        parent->nodes[moved + 1]->nodes[i]->parent = parent->nodes[moved + 1]; // set parents of right child node
    }

    if (node->parent->num_keys == (btree->branching)) {
        split(node->parent, helper); // but also add all the children of those nodes
    }
    return;
    
}
struct treenode * find_k(uint32_t key, void * helper, int find_dup) {
    struct btree *btree = helper;
    struct treenode *curr_node = btree->root;
    if (curr_node == NULL || helper == NULL) {
        return NULL;
    }
    int num = curr_node->num_keys;

    for (int i = 0; i < num; i++) {
        if (key == curr_node->keys[i]) { // if key was found, nothing to insert D:
            if (find_dup) {
                return curr_node;
            }
            return NULL;
        }
        if (curr_node->num_childs == 0 && !find_dup && (i == (num - 1))) { // if hits a leaf node and key was not found, return the leaf
            break;
        }
        if (curr_node->keys[i] > key && (curr_node->num_childs > 0)) {  // if it is greater, go to the left child of that node
            curr_node = curr_node->nodes[i];
            num = curr_node->num_keys;
            i = -1;
        } else if ((i == (num - 1)) && curr_node->num_childs != 0) {
            curr_node = curr_node->nodes[curr_node->num_childs - 1];
            // print_nd(curr_node);
            num = curr_node->num_keys;
            i = -1;
            
        }
    }
    
    if (find_dup) {
        return NULL;
    }
    return curr_node;
}

int btree_retrieve(uint32_t key, struct info * found, void * helper) {
    struct btree * btree = helper;
    pthread_mutex_lock(&btree->lock);

    if (found == NULL || helper == NULL) {
        pthread_mutex_unlock(&btree->lock);

        return 1;
    }
    struct treenode *node = find_k(key, helper, 1);
    if (node == NULL) {
        pthread_mutex_unlock(&btree->lock);
        return 1;
    }
    for (int i = 0; i < node->num_keys; i++) {
        if (node->keys[i] == key) {
            // struct info * yee = malloc(sizeof(struct info));

            found->data = node->key_info[i]->data;
            found->key[0] = node->key_info[i]->key[0];
            found->key[1] = node->key_info[i]->key[1];
            found->key[2] = node->key_info[i]->key[2];
            found->key[3] = node->key_info[i]->key[3];

            // printf("coy %ls\n", node->key_info[i]->key);
            // printf("coy %ls\n", found->key);

            found->nonce = node->key_info[i]->nonce;
            found->size = node->key_info[i]->size;
            // *found = *(yee);
            pthread_mutex_unlock(&btree->lock);
            return 0;
        }
    }
}

int btree_decrypt(uint32_t key, void * output, void * helper) {
    struct btree *btree = helper;
    pthread_mutex_lock(&btree->lock);
    struct treenode *node = find_k(key, helper, 1);
    if (node == NULL) {
        pthread_mutex_unlock(&btree->lock);
        return 1;
    }

    for (int i = 0; i < node->num_keys; i++) {
        if (node->keys[i] == key) {
            int num_blocks = (node->key_info[i]->size / 8) + ((node->key_info[i]->size % 8) != 0); // ceiling divides count by 64
            uint64_t * copy = calloc(num_blocks, sizeof(uint64_t));
            decrypt_tea_ctr_HACK(node->key_info[i]->data, node->key_info[i]->cache, node->key_info[i]->key, node->key_info[i]->nonce, copy, num_blocks);
            memcpy(output, copy, node->key_info[i]->size);
            free(copy);
            pthread_mutex_unlock(&btree->lock);
            return 0;
        }
    }
}

int btree_delete(uint32_t key, void * helper) {
    struct btree *btree = helper;
    pthread_mutex_lock(&btree->lock);

    if (find_k(key, helper, 0) != NULL) {
        pthread_mutex_unlock(&btree->lock);
        return 1;
    }
    pthread_mutex_unlock(&btree->lock);
    return 0;
}

uint64_t btree_export(void * helper, struct node ** list) {
    struct btree * btree = helper;
    pthread_mutex_lock(&btree->lock);
    if (helper == NULL || list == NULL) {
        pthread_mutex_unlock(&btree->lock);
        return 0;
    }
    *list = malloc(sizeof(struct node) * btree->num_nodes);
    int i = 0;
    preorder(btree->root, *list, &i, helper);
    pthread_mutex_unlock(&btree->lock);
    return btree->num_nodes;
}
// recursively traverses the tree starting from a node, and creates a list to put nodes into
void preorder(struct treenode * node, struct node * list, int * i, void *helper) {
    struct btree *btree = helper;
    if (node == NULL) {
        return;
    }
    uint32_t *copy = malloc(sizeof(uint32_t) * (btree->branching));
    memcpy(copy, node->keys, sizeof(uint32_t) * (btree->branching));
    list[*i].num_keys = node->num_keys;
    list[*i].keys = copy;

    // for (int i = 0; i < node->num_keys; i++ ) {
    //     printf("[INFO] PREORDER TRAVERSAL %d, number %d\n", copy[i], i);
    // }

    for (int j = 0; j < node->num_childs; j++) {
        *i += 1;
        preorder(node->nodes[j], list, i, helper);
    }
} 

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]) {
    uint32_t power = (uint32_t)pow(2, 32);
    uint32_t sum = 0;
    int delta = 0x9E3779B9;
    cipher[0] = plain[0];
    cipher[1] = plain[1];
    for (int i = 0; i < 1024; i++) {
        sum = (sum + delta) % power;
        uint32_t tmp1 = ((cipher[1] << 4) + key[0]) % power;
        uint32_t tmp2 = (cipher[1] + sum) % power;
        uint32_t tmp3 = ((cipher[1] >> 5) + key[1]) % power;
        cipher[0] = (cipher[0] + (tmp1^tmp2^tmp3)) % power;
        uint32_t tmp4 = ((cipher[0] << 4) + key[2]) % power;
        uint32_t tmp5 = (cipher[0] + sum) % power;
        uint32_t tmp6 = ((cipher[0] >> 5) + key[3]) % power;
        cipher[1] = (cipher[1] + (tmp4 ^ tmp5 ^ tmp6)) % power;
    }
    return;
}

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]) {
    uint32_t sum = 0xDDE6E400;
    uint32_t delta = 0x9E3779B9;
    uint32_t power = (uint32_t)pow(2, 32);
    for (int i = 0; i < 1024; i++) {
        uint32_t tmp4 = ((cipher[0] << 4) + key[2]) % power;
        uint32_t tmp5 = (cipher[0] + sum) % power;
        uint32_t tmp6 = ((cipher[0] >> 5) + key[3]) % power;
        cipher[1] = (cipher[1] - (tmp4 ^ tmp5 ^ tmp6)) % power;
        uint32_t tmp1 = ((cipher[1] << 4) + key[0]) % power;
        uint32_t tmp2 = (cipher[1] + sum) % power;
        uint32_t tmp3 = ((cipher[1] >> 5) + key[1]) % power;
        cipher[0] = (cipher[0] - (tmp1 ^ tmp2 ^ tmp3)) % power;
        sum = (sum - delta) % power;
        plain[0] = cipher[0];
        plain[1] = cipher[1];
    }
    return;
}

void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks) {
    for (int i = 0; i < num_blocks; i++) {
      uint64_t tmp1 = i ^ nonce;
      uint32_t tmp2[2] = {0};
      encrypt_tea((uint32_t *)&tmp1, tmp2, key);
      cipher[i] = plain[i] ^ *(uint64_t *)tmp2;
    }
    return;
}
void encrypt_tea_ctr_HACK(uint64_t * plain, uint64_t * cache, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks) {
    for (int i = 0; i < num_blocks; i++) {
      uint64_t tmp1 = i ^ nonce;
      uint32_t tmp2[2] = {0};
      encrypt_tea((uint32_t *)&tmp1, tmp2, key);
      cipher[i] = plain[i] ^ *(uint64_t *)tmp2;
      cache[i] = *(uint64_t *)tmp2;
    }
    return;
}

void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks) {
  for (int i = 0; i < num_blocks; i++) {
    uint64_t tmp1 = i ^ nonce;
    uint32_t tmp2[2] = {0};
    encrypt_tea((uint32_t *)&tmp1, tmp2, key);
    plain[i] = cipher[i] ^ *(uint64_t *)tmp2;
  }
  return;
}
void decrypt_tea_ctr_HACK(uint64_t * cipher, uint64_t * cache, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks) {
  for (int i = 0; i < num_blocks; i++) {
    plain[i] = cipher[i] ^ cache[i];
  }
  return;
}