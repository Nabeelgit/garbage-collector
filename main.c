#include "gc.h"
#include <stdio.h>
#include <stdlib.h>  // Include for malloc

int main() {
    gc_init();

    // Dynamically allocate ObjectMeta with zero pointer fields
    ObjectMeta *meta = malloc(sizeof(ObjectMeta) + sizeof(size_t) * 0);  // No extra space needed
    if (!meta) {
        fprintf(stderr, "Failed to allocate metadata.\n");
        exit(EXIT_FAILURE);
    }
    meta->count = 0;  // No pointer fields

    // Correct the call to gc_malloc by passing the size and the meta
    int *array = (int *)gc_malloc(sizeof(int) * 100, meta);
    gc_register_root((void **)&array);  // Register the root

    for (int i = 0; i < 100; ++i) {
        array[i] = i;
    }

    gc_collect();  // Trigger collection

    gc_unregister_root((void **)&array);  // Unregister the root
    gc_shutdown();

    free(meta);  // Clean up meta
    return 0;
}