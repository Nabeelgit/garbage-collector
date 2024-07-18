#ifndef GC_H
#define GC_H

#include <stddef.h>

typedef struct {
    size_t count;    // Number of pointer fields
    size_t offsets[]; // Variable-sized array of offsets
} ObjectMeta;

void gc_init();
void* gc_malloc(size_t size, ObjectMeta *meta);
void gc_collect();
void gc_free(void* ptr);
void gc_shutdown();
void gc_register_root(void **root);
void gc_unregister_root(void **root);

#endif