#include "gc.h"
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>  // Specific to Windows for advanced memory management
#include <stddef.h>

// No redefinition of ObjectMeta here

typedef struct GCHeader {
    struct GCHeader *next;
    size_t size;
    unsigned char marked;
    int generation;  // For generational GC
    ObjectMeta *meta; // Metadata about object layout
} GCHeader;

static GCHeader *head = NULL;
static HANDLE heapHandle;  // Handle to a private heap (Windows specific)

typedef struct GCRoot {
    void **pointer;
    struct GCRoot *next;
} GCRoot;

static GCRoot *root_set = NULL;

void gc_init() {
    heapHandle = HeapCreate(0, 0x010000, 0);  // Initial size 64K, grows as needed
    if (!heapHandle) {
        fprintf(stderr, "Failed to create heap.\n");
        exit(EXIT_FAILURE);
    }
}

void* gc_malloc(size_t size, ObjectMeta *meta) {
    // Allocate memory block from the private heap
    GCHeader* block = (GCHeader *)HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, sizeof(GCHeader) + size);
    if (!block) return NULL;

    block->size = size;
    block->next = head;
    block->marked = 0;
    block->generation = 0;  // New allocations are always in the youngest generation
    block->meta = meta;
    head = block;

    return (void*)(block + 1);
}

void gc_register_root(void **root) {
    GCRoot *new_root = (GCRoot *)malloc(sizeof(GCRoot));
    if (new_root) {
        new_root->pointer = root;
        new_root->next = root_set;
        root_set = new_root;
    }
}

void gc_unregister_root(void **root) {
    GCRoot **current = &root_set;
    while (*current) {
        if ((*current)->pointer == root) {
            GCRoot *temp = *current;
            *current = (*current)->next;
            free(temp);
            break;
        }
        current = &((*current)->next);
    }
}

void gc_mark_recursive(GCHeader *header) {
    if (header->marked == 1) return;
    header->marked = 1;

    if (header->meta) {
        // Iterate over each pointer field
        for (size_t i = 0; i < header->meta->count; i++) {
            size_t offset = header->meta->offsets[i];
            void **ptr = (void **)((char *)(header + 1) + offset);
            if (*ptr) {
                GCHeader *childHeader = (GCHeader *)(*ptr) - 1;
                if (childHeader->marked == 0) {
                    gc_mark_recursive(childHeader);
                }
            }
        }
    }
}

void gc_mark() {
    // Mark from roots
    for (GCRoot *root = root_set; root != NULL; root = root->next) {
        GCHeader *header = (GCHeader *)(*root->pointer) - 1;
        if (header && header->marked == 0) {
            gc_mark_recursive(header);
        }
    }
}

void gc_sweep() {
    GCHeader **current = &head;
    while (*current) {
        if (!(*current)->marked) {
            GCHeader *unreached = *current;
            *current = unreached->next;
            HeapFree(heapHandle, 0, unreached);
        } else {
            (*current)->marked = 0;  // Prepare for the next GC cycle
            current = &(*current)->next;
        }
    }
}

void gc_collect() {
    gc_mark();
    gc_sweep();
}

void gc_shutdown() {
    HeapDestroy(heapHandle);
}

void gc_free(void* ptr) {
    if (!ptr) return;

    // Get the header associated with this block
    GCHeader* header = (GCHeader*)ptr - 1;
    // Mark this block for collection
    header->marked = 0;

    // Optionally, you could immediately remove it from the list
    GCHeader **current = &head;
    while (*current) {
        if (*current == header) {
            *current = header->next;
            HeapFree(heapHandle, 0, header);
            break;
        }
        current = &(*current)->next;
    }
}