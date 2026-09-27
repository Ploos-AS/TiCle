#include "outqueue.h"

#include <string.h>

void ticle_outqueue_init(struct ticle_outqueue *queue) {
    if (!queue) return;
    queue->head = 0;
    queue->count = 0;
}

int ticle_outqueue_push(struct ticle_outqueue *queue, const char *line) {
    size_t len, index;
    if (!queue || !line) return -1;
    len = strlen(line);
    if (len == 0 || len >= TICLE_OUTQUEUE_LINE_MAX ||
        queue->count >= TICLE_OUTQUEUE_CAPACITY)
        return -1;
    index = (queue->head + queue->count) % TICLE_OUTQUEUE_CAPACITY;
    memcpy(queue->lines[index], line, len + 1);
    ++queue->count;
    return 0;
}

const char *ticle_outqueue_front(const struct ticle_outqueue *queue) {
    if (!queue || queue->count == 0) return NULL;
    return queue->lines[queue->head];
}

void ticle_outqueue_pop(struct ticle_outqueue *queue) {
    if (!queue || queue->count == 0) return;
    queue->head = (queue->head + 1) % TICLE_OUTQUEUE_CAPACITY;
    --queue->count;
}

size_t ticle_outqueue_count(const struct ticle_outqueue *queue) {
    return queue ? queue->count : 0;
}
