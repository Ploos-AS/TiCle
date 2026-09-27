#include "irc.h"

#include <string.h>

int irc_parse_message(const char *line, struct irc_message *msg) {
    char *p, *space;

    memset(msg, 0, sizeof *msg);
    if (!line || strlen(line) >= sizeof msg->storage) return -1;
    strcpy(msg->storage, line);
    p = msg->storage;

    if (*p == ':') {
        msg->prefix = ++p;
        space = strchr(p, ' ');
        if (!space) return -1;
        *space = '\0';
        p = space + 1;
        while (*p == ' ') ++p;
    }

    if (!*p) return -1;
    msg->command = p;
    space = strchr(p, ' ');
    if (!space) return 0;
    *space = '\0';
    p = space + 1;

    while (*p && msg->nparams < TICLE_IRC_MAX_PARAMS) {
        while (*p == ' ') ++p;
        if (!*p) break;
        if (*p == ':') {
            msg->params[msg->nparams++] = p + 1;
            break;
        }
        msg->params[msg->nparams++] = p;
        space = strchr(p, ' ');
        if (!space) break;
        *space = '\0';
        p = space + 1;
    }
    return 0;
}
