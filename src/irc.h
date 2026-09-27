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

int irc_parse_message(const char *line, struct irc_message *msg);

#endif
