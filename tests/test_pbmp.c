#include "../src/pbmp.h"
#include <assert.h>
#include <string.h>

int main(void) {
    struct ticle_pbmp_state s = {"ticle", "irc.example", 0};
    char out[2048];
    assert(ticle_pbmp_response("{\"pbmp\":1,\"type\":\"request\",\"id\":\"1\",\"method\":\"pbmp.info\",\"params\":{}}", &s, out, sizeof out) == 0);
    assert(strstr(out, "\"version\":1"));
    assert(strstr(out, "\"name\":\"ticle\""));
    assert(ticle_pbmp_response("{\"pbmp\":1,\"type\":\"request\",\"id\":\"2\",\"method\":\"bot.info\",\"params\":{}}", &s, out, sizeof out) == 0);
    assert(strstr(out, "\"id\":\"ticle\""));
    assert(strstr(out, "\"state\":\"stopped\""));
    assert(ticle_pbmp_response("{\"pbmp\":1,\"type\":\"request\",\"id\":\"3\",\"method\":\"networks.list\",\"params\":{}}", &s, out, sizeof out) == 0);
    assert(strstr(out, "\"id\":\"irc.example\""));
    assert(strstr(out, "\"state\":\"disconnected\""));
    assert(ticle_pbmp_response("{\"pbmp\":1,\"type\":\"request\",\"id\":\"4\",\"method\":\"capabilities.list\",\"params\":{}}", &s, out, sizeof out) == 0);
    assert(strstr(out, "networks.list"));
    return 0;
}
