#ifndef TICLE_CONFIG_H
#define TICLE_CONFIG_H

#define TICLE_CONFIG_VALUE_MAX 512

struct ticle_config {
    char host[TICLE_CONFIG_VALUE_MAX];
    char port[TICLE_CONFIG_VALUE_MAX];
    char nick[TICLE_CONFIG_VALUE_MAX];
    char user[TICLE_CONFIG_VALUE_MAX];
    char realname[TICLE_CONFIG_VALUE_MAX];
    char script[TICLE_CONFIG_VALUE_MAX];
};

int ticle_config_load(const char *path, struct ticle_config *config);

#endif
