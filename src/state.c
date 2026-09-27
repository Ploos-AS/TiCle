#include "state.h"

const char *ticle_state_name(enum ticle_connection_state state) {
    switch (state) {
    case TICLE_DISCONNECTED: return "disconnected";
    case TICLE_CONNECTING: return "connecting";
    case TICLE_REGISTERING: return "registering";
    case TICLE_ONLINE: return "online";
    case TICLE_STOPPING: return "stopping";
    default: return "unknown";
    }
}

int ticle_state_can_transition(enum ticle_connection_state from,
                               enum ticle_connection_state to) {
    if (from == to) return 1;
    if (to == TICLE_STOPPING) return from != TICLE_STOPPING;

    switch (from) {
    case TICLE_DISCONNECTED:
        return to == TICLE_CONNECTING;
    case TICLE_CONNECTING:
        return to == TICLE_REGISTERING || to == TICLE_DISCONNECTED;
    case TICLE_REGISTERING:
        return to == TICLE_ONLINE || to == TICLE_DISCONNECTED;
    case TICLE_ONLINE:
        return to == TICLE_DISCONNECTED;
    case TICLE_STOPPING:
    default:
        return 0;
    }
}
