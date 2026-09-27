#ifndef TICLE_NICK_H
#define TICLE_NICK_H

#include <stddef.h>

int ticle_nick_candidate(char *out, size_t out_size,
                         const char *base, unsigned int collision);

#endif
