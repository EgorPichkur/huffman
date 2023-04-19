#include "priority_queue.h"
#include <stdbool.h>

void init_heap(binary_heap_ptr heap, uint16_t max_size) {
    CHKPTR(heap);
    heap->size = 0;
    heap->capacity = max_size;
    heap->heap_array = malloc(max_size * sizeof(frequency_node_ptr));
    CHKPTR(heap->heap_array);
}

void free_heap(binary_heap_ptr *heap) {
    CHKPTR(heap);
    FREE((*heap)->heap_array);
    FREE(*heap);
}

int8_t insert(binary_heap_ptr heap, frequency_node_ptr value) {
    if (heap->size == heap->capacity) {
        DBG_LOG("%s\n", "HEAP OVERFLOW!\n");
        return -1;
    }
    heap->size++;
    uint16_t i = heap->size - 1;
    heap->heap_array[i] = value;

    while ((i != 0) && (heap->heap_array[PARENT(i)]->frequency > heap->heap_array[i]->frequency)) {
        SWAP(heap->heap_array[i], heap->heap_array[PARENT(i)]);
        i = PARENT(i);
    }
    return 0;
}

frequency_node_ptr extract_min(binary_heap_ptr heap) {
    if (heap->size == 0) {
        DBG_LOG("%s\n", "HEAP IS EMPTY!\n");
        return NULL;
    }
    if (heap->size == 1) {
        heap->size--;
        return heap->heap_array[0];
    }
    frequency_node_ptr root = heap->heap_array[0];
    heap->heap_array[0] = heap->heap_array[heap->size - 1];
    heap->size--;
    min_heapify(heap, 0);

    return root;
}

void min_heapify(binary_heap_ptr heap, uint16_t index) {
    uint16_t l = 2 * index + 1;
    uint16_t r = 2 * index + 2;
    uint16_t smallest = index;
    if ((l < heap->size) && (heap->heap_array[l]->frequency < heap->heap_array[index]->frequency)) {
        smallest = l;
    }
    if ((r < heap->size) && (heap->heap_array[r]->frequency < heap->heap_array[smallest]->frequency)) {
        smallest = r;
    }
    if (smallest != index) {
        SWAP(heap->heap_array[index], heap->heap_array[smallest]);
        min_heapify(heap, smallest);
    }
}
