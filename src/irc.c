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

    while (*p) {
        while (*p == ' ') ++p;
        if (!*p) break;
        if (msg->nparams >= TICLE_IRC_MAX_PARAMS) return -1;
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


void irc_parse_identity(const char *prefix, struct irc_identity *identity) {
    char copy[TICLE_IRC_BUFSIZE];
    char *bang, *at;

    memset(identity, 0, sizeof *identity);
    if (!prefix || strlen(prefix) >= sizeof copy) return;
    strcpy(copy, prefix);

    bang = strchr(copy, '!');
    at = bang ? strchr(bang + 1, '@') : NULL;
    if (!bang || !at) return;

    *bang = '\0';
    *at = '\0';
    strcpy(identity->nick, copy);
    strcpy(identity->user, bang + 1);
    strcpy(identity->host, at + 1);
}
