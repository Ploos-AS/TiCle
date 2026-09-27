#include "../src/backoff.h"

#include <stdio.h>
#include <stdlib.h>

static void expect(unsigned int got, unsigned int want, const char *name) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s: got %u, want %u\n", name, got, want);
        exit(1);
    }
}

int main(void) {
    expect(ticle_backoff_seconds(0), 1, "attempt 0");
    expect(ticle_backoff_seconds(1), 2, "attempt 1");
    expect(ticle_backoff_seconds(2), 4, "attempt 2");
    expect(ticle_backoff_seconds(3), 8, "attempt 3");
    expect(ticle_backoff_seconds(4), 16, "attempt 4");
    expect(ticle_backoff_seconds(5), 30, "attempt 5");
    expect(ticle_backoff_seconds(100), 30, "bounded");
    puts("PASS: backoff tests");
    return 0;
}
