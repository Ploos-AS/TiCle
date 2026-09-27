#include "../src/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void expect(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void) {
    struct ticle_config c;
    const char *path = "tests/.test-config.tmp";
    FILE *fp = fopen(path, "w");
    expect(fp != NULL, "create config");
    fputs("# TiCle test config\n"
          "host = irc.example.org\n"
          "port=6667\n"
          "nick=TiCle\n"
          "script=scripts/example.tcl\n", fp);
    fclose(fp);

    expect(ticle_config_load(path, &c) == 0, "load config");
    expect(strcmp(c.host, "irc.example.org") == 0, "host");
    expect(strcmp(c.port, "6667") == 0, "port");
    expect(strcmp(c.nick, "TiCle") == 0, "nick");
    expect(strcmp(c.user, "TiCle") == 0, "default user");
    expect(strcmp(c.realname, "TiCle IRC bot") == 0, "default realname");
    unlink(path);

    fp = fopen(path, "w");
    expect(fp != NULL, "create invalid config");
    fputs("host=irc.example.org\nport=6667\nnick=TiCle\nscript=x.tcl\nunknown=yes\n", fp);
    fclose(fp);
    expect(ticle_config_load(path, &c) == -1, "unknown key rejected");
    unlink(path);

    puts("PASS: config tests");
    return 0;
}
