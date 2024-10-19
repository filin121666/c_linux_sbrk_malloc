#include "memory_allocator.h"
#include <unistd.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>

typedef char ALIGN[16];

union header
{
    struct
    {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    ALIGN stub;
};
typedef union header header_t;

header_t *head = NULL, *tail = NULL;
pthread_mutex_t global_malloc_mutex;

header_t *get_free_block(size_t size)
{
    header_t *current_block = head;
    while (current_block)
    {
        if (current_block->s.is_free && current_block->s.size >= size)
        {
            return current_block;
        }
        current_block = current_block->s.next;
    }
    return NULL;
}

void *malloc(size_t size)
{
    size_t total_size;
    void *block;
    header_t *header;

    if (!size)
    {
        return NULL;
    }

    pthread_mutex_lock(&global_malloc_mutex);
    header = get_free_block(size);
    if (header)
    {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_mutex);
        return (void *)(header + 1);
    }

    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void *)-1)
    {
        pthread_mutex_unlock(&global_malloc_mutex);
        return NULL;
    }

    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    if (!head)
    {
        head = header;
    }
    if (tail)
    {
        tail->s.next = header;
    }
    tail = header;
    pthread_mutex_unlock(&global_malloc_mutex);
    return (void *)(header + 1);
}

void free(void *block)
{
    header_t *header, *tmp;
    void *program_break;

    if (!block)
    {
        return;
    }

    pthread_mutex_lock(&global_malloc_mutex);
    header = (header_t *)block - 1;

    program_break = sbrk(0);
    if ((char *)block + header->s.size == program_break)
    {
        if (head == tail) {
            head = tail = NULL;
        } else {
            header_t *tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        sbrk(0 - header->s.size - sizeof(header_t));
        pthread_mutex_unlock(&global_malloc_mutex);
        return;
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_mutex);
}

void *calloc(size_t count_elements, size_t element_size)
{
    size_t size;
    void *block;

    if (!count_elements || !element_size)
    {
        return NULL;
    }

    size = count_elements * element_size;

    if (element_size != size / count_elements) {
        return NULL;
    }

    block = malloc(size);
    if (!block) {
        return NULL;
    }
    memset(block, 0, size);
    return block;
}

void *realloc(void *block, size_t size)
{
    header_t *header;
    void *new_block;

    if (!block || !size) {
        return malloc(size);
    }

    header = (header_t*) block - 1;
    if (header->s.size >= size) {
        return block;
    }

    new_block = malloc(size);
    if (new_block) {
        memcpy(new_block, block, header->s.size);
        free(block);
    }
    return new_block;
}
