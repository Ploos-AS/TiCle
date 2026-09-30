#include "pbmp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

static int field(const char *json, const char *name, char *out, size_t n) {
    char key[64];
    snprintf(key, sizeof key, "\"%s\"", name);
    const char *p = strstr(json, key);
    if (!p || !(p = strchr(p + strlen(key), ':'))) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p++ != '"') return -1;
    size_t z = 0;
    while (*p && *p != '"' && z + 1 < n) {
        if (*p == '\\') return -1;
        out[z++] = *p++;
    }
    if (*p != '"') return -1;
    out[z] = 0;
    return 0;
}

static int valid_request(const char *q) {
    return q && strlen(q) <= 4096 &&
           strstr(q, "\"pbmp\":1") &&
           strstr(q, "\"type\":\"request\"") &&
           strstr(q, "\"params\":{");
}

int ticle_pbmp_response(const char *q, const struct ticle_pbmp_state *s,
                        char *out, size_t n) {
    char id[128], method[128], result[1024];
    if (!valid_request(q) || field(q, "id", id, sizeof id) ||
        field(q, "method", method, sizeof method)) return -1;
    if (!strcmp(method, "pbmp.info")) {
        snprintf(result, sizeof result,
                 "{\"version\":1,\"implementation\":{\"name\":\"ticle\",\"version\":\"0.1.0\"}}");
    } else if (!strcmp(method, "capabilities.list")) {
        snprintf(result, sizeof result,
                 "{\"capabilities\":[\"pbmp.info\",\"capabilities.list\",\"bot.info\",\"networks.list\"]}");
    } else if (!strcmp(method, "bot.info")) {
        snprintf(result, sizeof result,
                 "{\"bot\":{\"id\":\"%s\",\"implementation\":{\"name\":\"ticle\",\"version\":\"0.1.0\"},\"state\":\"%s\"}}",
                 s->nick, s->connected ? "running" : "stopped");
    } else if (!strcmp(method, "networks.list")) {
        snprintf(result, sizeof result,
                 "{\"networks\":[{\"id\":\"%s\",\"name\":\"%s\",\"state\":\"%s\"}]}",
                 s->network, s->network, s->connected ? "connected" : "disconnected");
    } else {
        return snprintf(out, n,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":false,\"error\":{\"code\":\"not_supported\",\"message\":\"method not supported\"}}\n", id) >= (int)n ? -1 : 0;
    }
    return snprintf(out, n,
        "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,\"result\":%s}\n",
        id, result) >= (int)n ? -1 : 0;
}

int ticle_pbmp_serve(const char *path, const struct ticle_pbmp_state *s) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_un a = {0};
    a.sun_family = AF_UNIX;
    if (!path || strlen(path) >= sizeof a.sun_path) { close(fd); return -1; }
    snprintf(a.sun_path, sizeof a.sun_path, "%s", path);
    unlink(path);
    if (bind(fd, (struct sockaddr *)&a, sizeof a) || chmod(path, 0600) || listen(fd, 4)) {
        close(fd); unlink(path); return -1;
    }
    for (;;) {
        int client = accept(fd, NULL, NULL);
        if (client < 0) continue;
        char in[4097] = {0}, out[2048];
        ssize_t z = read(client, in, sizeof in - 1);
        if (z > 0 && memchr(in, '\n', (size_t)z) &&
            !ticle_pbmp_response(in, s, out, sizeof out))
            (void)write(client, out, strlen(out));
        close(client);
    }
}
