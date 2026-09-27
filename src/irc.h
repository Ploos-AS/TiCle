#ifndef TICLE_IRC_H
#define TICLE_IRC_H

#define TICLE_IRC_BUFSIZE 4096
#define TICLE_IRC_MAX_PARAMS 15

struct irc_message {
    char storage[TICLE_IRC_BUFSIZE];
    char *prefix;
    char *command;
    char *params[TICLE_IRC_MAX_PARAMS];
    int nparams;
};

struct irc_identity {
    char nick[TICLE_IRC_BUFSIZE];
    char user[TICLE_IRC_BUFSIZE];
    char host[TICLE_IRC_BUFSIZE];
};

int irc_parse_message(const char *line, struct irc_message *msg);
void irc_parse_identity(const char *prefix, struct irc_identity *identity);

#endif
