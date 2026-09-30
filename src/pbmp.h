#ifndef TICLE_PBMP_H
#define TICLE_PBMP_H
#include <stddef.h>

struct ticle_pbmp_state {
    const char *nick;
    const char *network;
    int connected;
};

int ticle_pbmp_response(const char *request, const struct ticle_pbmp_state *state,
                        char *out, size_t out_size);
int ticle_pbmp_serve(const char *socket_path, const struct ticle_pbmp_state *state);
#endif
