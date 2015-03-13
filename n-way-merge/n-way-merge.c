#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define KEYSIZE   4
#define CHUNKSIZE (512*1024*KEYSIZE)
#define N         50

static int64_t buffer[N+1][CHUNKSIZE];

int cmp(int64_t **key, int a, int b) {
    int64_t *ka = key[a];
    int64_t *kb = key[b];
    int i;
    for (i = 0; i < KEYSIZE; i++) {
        if (ka[i] < kb[i]) return 1;
        if (ka[i] > kb[i]) return 0;
    }
    return 1;
}

static void
heap_dump(int *heap, int64_t **key, int size) {
    int i, j;
    for (i = 0; i < size; i++) {
        int pivot = heap[i];
        fprintf(stderr, "%s%d [", (i ? ", " : ""), pivot);
        for (j = 0; j < KEYSIZE; j++) {
            fprintf(stderr, "%s%ld", (j ? ", " : ""), key[pivot][j]);
        }
        fprintf(stderr, "]");
    }
    fprintf(stderr, "\n");
}

static void
heap_init(int *heap, int64_t **key, int size) {
    int i;
    for (i = 0; i < size; i++) {
        unsigned int index, parent;
        for (index = i; index; index = parent) {
            parent = (index - 1) >> 1;
            if (cmp(key, heap[parent], i)) break;
            heap[index] = heap[parent];
        }
        heap[index] = i;
        // heap_dump(heap, key, i + 1);
    }
}

static int
heap_top(int *heap, int size) {
    return *heap;
}

static void
heap_sink_top(int *heap, int64_t **key, int pivot, int size) {
    int index, swap, other;
    for (index = 0; 1; index = swap) {
        swap = (index << 1) + 1;
        if (swap >= size) break;
        other = swap + 1;
        if ((other < size) && cmp(key, heap[other], heap[swap])) swap = other;
        if (cmp(key, pivot, heap[swap])) break;
        heap[index] = heap[swap];
    }
    heap[index] = pivot; 
}

static void
heap_pop(int *heap, int64_t **key, int new_size) {
    heap_sink_top(heap, key, heap[new_size], new_size);
}

void
flush_buffer(uint64_t *buffer, uint64_t *top) {
    char *start = (char *)buffer;
    char *end = (char *)top;
    while (start < end) {
        ssize_t bytes = write(1, start, (end - start));
        if (bytes > 0)
            start += bytes;
        else if ((errno != EINTR) && (errno != EAGAIN)) {
            perror("failed to write");
            exit(1);
        }
    }
    fprintf(stderr, "%ld bytes written\n", start - (char *)buffer); fflush(stderr);
}

uint64_t *
fill_buffer(int fd, uint64_t *buffer, uint64_t *top) {
    char *start = (char *)buffer;
    char *end = (char *)top;
    while (start < end) {
        ssize_t bytes = read(fd, start, end - start);
        if (bytes > 0)
            start += bytes;
        else if (bytes == 0)
            break;
        else if ((errno != EINTR) && (errno != EAGAIN)) {
            perror("failed to read");
            exit(1);
        }
    }
    fprintf(stderr, "%ld bytes read from fd %d\n", start - (char *)buffer, fd); fflush(stderr);
    return (uint64_t *)start;
}

int
main(int argc, char *argv[]) {
    int fd[N];
    int64_t *out_current = buffer[N];
    int64_t *out_end = buffer[N+1];
    int64_t *key[N];
    int64_t *key_end[N];
    int heap[N];
    int size;
    int top, i;

    if (argc - 1 > N) {
        fprintf(stderr, "too many arguments\n");
        exit(1);
    }

    for (i = 1, size = 0; i < argc; i++) {
        argv++;
        fd[size] = open(*argv, O_RDONLY);
        if (fd[size] < 0) {
            perror(*argv);
            exit(1);
        }
        
        key[size] = buffer[size];
        key_end[size] = fill_buffer(fd[size], key[size], buffer[size+1]);
        /* ignore empty files */
        if (key[size] < key_end[size])
            size++;
    }
    
    heap_init(heap, key, size);

    while (size) {
        // heap_dump(heap, key, size);
        top = heap_top(heap, size);
        for (i = 0; i < KEYSIZE; i++) {
            *(out_current++) = *(key[top]++);
        }
        if (out_current >= out_end) {
            flush_buffer(buffer[N], out_current);
            out_current = buffer[N];
        }
        if (key[top] >= key_end[top]) {
            key[top] = buffer[top];
            key_end[top] = fill_buffer(fd[top], key[top], buffer[top+1]);
            if (key[top] == key_end[top]) {
                heap_pop(heap, key, --size);
                continue;
            }
        }
        heap_sink_top(heap, key, top, size);
    }
    flush_buffer(buffer[N], out_current);
}
