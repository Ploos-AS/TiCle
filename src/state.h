#ifndef TICLE_STATE_H
#define TICLE_STATE_H

enum ticle_connection_state {
    TICLE_DISCONNECTED = 0,
    TICLE_CONNECTING,
    TICLE_REGISTERING,
    TICLE_ONLINE,
    TICLE_STOPPING
};

const char *ticle_state_name(enum ticle_connection_state state);
int ticle_state_can_transition(enum ticle_connection_state from,
                               enum ticle_connection_state to);

#endif
