#ifndef TICLE_OUTQUEUE_H
#define TICLE_OUTQUEUE_H

#include <stddef.h>

#define TICLE_OUTQUEUE_CAPACITY 64
#define TICLE_OUTQUEUE_LINE_MAX 512

struct ticle_outqueue {
    char lines[TICLE_OUTQUEUE_CAPACITY][TICLE_OUTQUEUE_LINE_MAX];
    size_t head;
    size_t count;
};

void ticle_outqueue_init(struct ticle_outqueue *queue);
int ticle_outqueue_push(struct ticle_outqueue *queue, const char *line);
const char *ticle_outqueue_front(const struct ticle_outqueue *queue);
void ticle_outqueue_pop(struct ticle_outqueue *queue);
size_t ticle_outqueue_count(const struct ticle_outqueue *queue);

#endif
