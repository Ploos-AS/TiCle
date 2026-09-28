#ifndef TICLE_IRC_H
#define TICLE_IRC_H

#define TICLE_IRC_BUFSIZE 4096
#define TICLE_IRC_MAX_PARAMS 15
#define TICLE_IRC_MAX_TAGS 32

struct irc_tag {
    char *key;
    char *value;
};

struct irc_message {
    char storage[TICLE_IRC_BUFSIZE];
    struct irc_tag tags[TICLE_IRC_MAX_TAGS];
    int ntags;
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
int irc_format_registration(char *nick_line, unsigned long nick_size,
                            char *user_line, unsigned long user_size,
                            const char *nick, const char *user,
                            const char *realname);

#endif
