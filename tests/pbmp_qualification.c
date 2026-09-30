#include "../src/pbmp.h"
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    const char *path = getenv("TICLE_PBMP_SOCKET");
    if (!path || !*path) return 2;
    struct ticle_pbmp_state state = {"ticle-qualification", "irc.example.invalid", 0};
    if (ticle_pbmp_serve(path, &state)) { perror("PBMP"); return 1; }
    return 0;
}
