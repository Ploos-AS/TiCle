#include "irc.h"

#include <stdio.h>
#include <string.h>

static void irc_unescape_tag_value(char *value) {
    char *src = value, *dst = value;
    while (*src) {
        if (*src != '\\') {
            *dst++ = *src++;
            continue;
        }
        ++src;
        if (!*src) break;
        switch (*src) {
        case ':': *dst++ = ';'; break;
        case 's': *dst++ = ' '; break;
        case '\\': *dst++ = '\\'; break;
        case 'r': *dst++ = '\r'; break;
        case 'n': *dst++ = '\n'; break;
        default: *dst++ = *src; break;
        }
        ++src;
    }
    *dst = '\0';
}

int irc_parse_message(const char *line, struct irc_message *msg) {
    char *p, *space;

    memset(msg, 0, sizeof *msg);
    if (!line || strlen(line) >= sizeof msg->storage) return -1;
    strcpy(msg->storage, line);
    p = msg->storage;

    if (*p == '@') {
        char *tags = ++p;
        space = strchr(p, ' ');
        if (!space) return -1;
        *space = '\0';
        p = space + 1;
        while (*tags) {
            char *end, *eq;
            if (msg->ntags >= TICLE_IRC_MAX_TAGS) return -1;
            end = strchr(tags, ';');
            if (end) *end = '\0';
            eq = strchr(tags, '=');
            msg->tags[msg->ntags].key = tags;
            if (eq) {
                *eq = '\0';
                msg->tags[msg->ntags].value = eq + 1;
                irc_unescape_tag_value(msg->tags[msg->ntags].value);
            }
            if (!*msg->tags[msg->ntags].key) return -1;
            ++msg->ntags;
            if (!end) break;
            tags = end + 1;
        }
        while (*p == ' ') ++p;
    }

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
    if (strlen(copy) >= sizeof identity->nick ||
        strlen(bang + 1) >= sizeof identity->user ||
        strlen(at + 1) >= sizeof identity->host) return;
    strcpy(identity->nick, copy);
    strcpy(identity->user, bang + 1);
    strcpy(identity->host, at + 1);
}


int irc_format_registration(char *nick_line, unsigned long nick_size,
                            char *user_line, unsigned long user_size,
                            const char *nick, const char *user,
                            const char *realname) {
    int n1, n2;
    if (!nick_line || !user_line || !nick || !user || !realname ||
        !*nick || !*user || !*realname ||
        strchr(nick, ' ') || strchr(user, ' ') ||
        strchr(nick, '\r') || strchr(nick, '\n') ||
        strchr(user, '\r') || strchr(user, '\n') ||
        strchr(realname, '\r') || strchr(realname, '\n')) return -1;

    n1 = snprintf(nick_line, (size_t)nick_size, "NICK %s", nick);
    n2 = snprintf(user_line, (size_t)user_size, "USER %s 0 * :%s", user, realname);
    if (n1 < 0 || n2 < 0 || (unsigned long)n1 >= nick_size || (unsigned long)n2 >= user_size)
        return -1;
    return 0;
}
