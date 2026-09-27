#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char *trim(char *s) {
    char *end;
    while (isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

static int set_value(char *dst, size_t size, const char *value) {
    size_t n = strlen(value);
    if (n == 0 || n >= size || strchr(value, '\r') || strchr(value, '\n')) return -1;
    memcpy(dst, value, n + 1);
    return 0;
}

int ticle_config_load(const char *path, struct ticle_config *config) {
    FILE *fp;
    char line[1024];

    if (!path || !config) return -1;
    memset(config, 0, sizeof *config);
    fp = fopen(path, "r");
    if (!fp) return -1;

    while (fgets(line, sizeof line, fp)) {
        char *key, *value, *eq;
        if (!strchr(line, '\n') && !feof(fp)) { fclose(fp); return -1; }
        key = trim(line);
        if (!*key || *key == '#') continue;
        eq = strchr(key, '=');
        if (!eq) { fclose(fp); return -1; }
        *eq = '\0';
        value = trim(eq + 1);
        key = trim(key);

        if (strcmp(key, "host") == 0) {
            if (set_value(config->host, sizeof config->host, value) < 0) goto fail;
        } else if (strcmp(key, "port") == 0) {
            if (set_value(config->port, sizeof config->port, value) < 0) goto fail;
        } else if (strcmp(key, "nick") == 0) {
            if (set_value(config->nick, sizeof config->nick, value) < 0) goto fail;
        } else if (strcmp(key, "user") == 0) {
            if (set_value(config->user, sizeof config->user, value) < 0) goto fail;
        } else if (strcmp(key, "realname") == 0) {
            if (set_value(config->realname, sizeof config->realname, value) < 0) goto fail;
        } else if (strcmp(key, "script") == 0) {
            if (set_value(config->script, sizeof config->script, value) < 0) goto fail;
        } else {
            goto fail;
        }
    }
    fclose(fp);

    if (!config->host[0] || !config->port[0] || !config->nick[0] || !config->script[0])
        return -1;
    if (!config->user[0]) {
        if (set_value(config->user, sizeof config->user, config->nick) < 0) return -1;
    }
    if (!config->realname[0]) {
        if (set_value(config->realname, sizeof config->realname, "TiCle IRC bot") < 0) return -1;
    }
    return 0;

fail:
    fclose(fp);
    return -1;
}
