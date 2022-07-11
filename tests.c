#include "btreestore.h"
#include <stdio.h>

int main() {
    // Your own testing code here
    void * helper = init_store(4, 4);
    // struct treenode *btree = helper;
    // btree->root = NULL;
    printf("%d", find_k(2, helper) == NULL);

    close_store(helper);

    return 0;
}
