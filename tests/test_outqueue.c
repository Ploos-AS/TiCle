#include "../src/outqueue.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    struct ticle_outqueue q;
    size_t i;
    ticle_outqueue_init(&q);
    if (ticle_outqueue_count(&q) != 0 || ticle_outqueue_front(&q) != NULL) return 1;
    if (ticle_outqueue_push(&q, "PRIVMSG #test :one") < 0 ||
        ticle_outqueue_push(&q, "PRIVMSG #test :two") < 0) return 1;
    if (strcmp(ticle_outqueue_front(&q), "PRIVMSG #test :one") != 0) return 1;
    ticle_outqueue_pop(&q);
    if (strcmp(ticle_outqueue_front(&q), "PRIVMSG #test :two") != 0) return 1;
    ticle_outqueue_pop(&q);
    for (i = 0; i < TICLE_OUTQUEUE_CAPACITY; ++i)
        if (ticle_outqueue_push(&q, "NOTICE x :queued") < 0) return 1;
    if (ticle_outqueue_push(&q, "NOTICE x :overflow") == 0) return 1;
    puts("PASS: outbound queue tests");
    return 0;
}
