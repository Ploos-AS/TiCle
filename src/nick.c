#include "nick.h"

#include <string.h>

int ticle_nick_candidate(char *out, size_t out_size,
                         const char *base, unsigned int collision) {
    size_t base_len, needed, i;
    if (!out || !base || !*base || out_size == 0) return -1;
    base_len = strlen(base);
    if ((size_t)collision > (size_t)-1 - base_len - 1U) return -1;
    needed = base_len + (size_t)collision + 1U;
    if (needed > out_size) return -1;
    memcpy(out, base, base_len);
    for (i = 0; i < collision; ++i) out[base_len + i] = '_';
    out[base_len + collision] = '\0';
    return 0;
}
