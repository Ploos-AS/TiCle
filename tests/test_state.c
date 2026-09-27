#include "../src/state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void) {
    expect(strcmp(ticle_state_name(TICLE_DISCONNECTED), "disconnected") == 0, "state name");
    expect(ticle_state_can_transition(TICLE_DISCONNECTED, TICLE_CONNECTING), "connect");
    expect(ticle_state_can_transition(TICLE_CONNECTING, TICLE_REGISTERING), "connected");
    expect(ticle_state_can_transition(TICLE_REGISTERING, TICLE_ONLINE), "registered");
    expect(ticle_state_can_transition(TICLE_ONLINE, TICLE_DISCONNECTED), "connection lost");
    expect(ticle_state_can_transition(TICLE_CONNECTING, TICLE_DISCONNECTED), "connect failed");
    expect(ticle_state_can_transition(TICLE_REGISTERING, TICLE_DISCONNECTED), "registration lost");
    expect(ticle_state_can_transition(TICLE_ONLINE, TICLE_STOPPING), "stop online");
    expect(ticle_state_can_transition(TICLE_DISCONNECTED, TICLE_STOPPING), "stop disconnected");
    expect(!ticle_state_can_transition(TICLE_DISCONNECTED, TICLE_ONLINE), "reject disconnected to online");
    expect(!ticle_state_can_transition(TICLE_ONLINE, TICLE_REGISTERING), "reject online to registering");
    expect(!ticle_state_can_transition(TICLE_STOPPING, TICLE_DISCONNECTED), "stopping is terminal");
    puts("PASS: state tests");
    return 0;
}
