#include "../src/nick.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect(unsigned int n, const char *want) {
    char out[32];
    if (ticle_nick_candidate(out, sizeof out, "TiCle", n) < 0 ||
        strcmp(out, want) != 0) {
        fprintf(stderr, "FAIL: collision %u\n", n);
        exit(1);
    }
}

int main(void) {
    char tiny[6];
    expect(0, "TiCle");
    expect(1, "TiCle_");
    expect(2, "TiCle__");
    if (ticle_nick_candidate(tiny, sizeof tiny, "TiCle", 1) == 0) {
        fprintf(stderr, "FAIL: accepted truncated nick\n");
        return 1;
    }
    if (ticle_nick_candidate(NULL, 0, "TiCle", 0) == 0) return 1;
    puts("PASS: nick tests");
    return 0;
}
