#include "../src/irc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void expect_str(const char *actual, const char *expected, const char *message) {
    expect(actual != NULL, message);
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "FAIL: %s: expected <%s>, got <%s>\n", message, expected, actual);
        exit(1);
    }
}

int main(void) {
    struct irc_message m;

    expect(irc_parse_message(":alice!u@example PRIVMSG #test :hello world", &m) == 0, "PRIVMSG parses");
    expect_str(m.prefix, "alice!u@example", "PRIVMSG prefix");
    expect_str(m.command, "PRIVMSG", "PRIVMSG command");
    expect(m.nparams == 2, "PRIVMSG parameter count");
    expect_str(m.params[0], "#test", "PRIVMSG target");
    expect_str(m.params[1], "hello world", "PRIVMSG trailing text");

    expect(irc_parse_message(":alice!u@example JOIN #test", &m) == 0, "JOIN parses");
    expect_str(m.command, "JOIN", "JOIN command");
    expect(m.nparams == 1, "JOIN parameter count");
    expect_str(m.params[0], "#test", "JOIN channel");

    expect(irc_parse_message(":irc.example 001 TiCle :Welcome", &m) == 0, "numeric parses");
    expect_str(m.prefix, "irc.example", "numeric server prefix");
    expect_str(m.command, "001", "numeric command");
    expect(m.nparams == 2, "numeric parameter count");
    expect_str(m.params[1], "Welcome", "numeric trailing parameter");

    expect(irc_parse_message("PING :token", &m) == 0, "PING parses");
    expect(m.prefix == NULL, "PING has no prefix");
    expect_str(m.command, "PING", "PING command");
    expect_str(m.params[0], "token", "PING token");

    expect(irc_parse_message("QUIT", &m) == 0, "command-only line parses");
    expect_str(m.command, "QUIT", "command-only command");
    expect(m.nparams == 0, "command-only has no parameters");

    expect(irc_parse_message("", &m) == -1, "empty line rejected");
    expect(irc_parse_message(":broken", &m) == -1, "prefix without command rejected");
    expect(irc_parse_message(NULL, &m) == -1, "NULL line rejected");

    expect(irc_parse_message("PRIVMSG    #test    :spaced text", &m) == 0, "extra spaces parse");
    expect(m.nparams == 2, "extra spaces parameter count");
    expect_str(m.params[0], "#test", "extra spaces target");
    expect_str(m.params[1], "spaced text", "extra spaces trailing text");

    expect(irc_parse_message("CMD a b c d e f g h i j k l m n o", &m) == 0, "maximum parameters accepted");
    expect(m.nparams == TICLE_IRC_MAX_PARAMS, "maximum parameter count");

    expect(irc_parse_message("CMD a b c d e f g h i j k l m n o p", &m) == -1, "too many parameters rejected");

    {
        char overlong[TICLE_IRC_BUFSIZE + 1];
        memset(overlong, 'A', sizeof overlong - 1);
        overlong[sizeof overlong - 1] = '\0';
        expect(irc_parse_message(overlong, &m) == -1, "overlong input rejected");
    }

    {
        struct irc_identity identity;
        irc_parse_identity("alice!user@example.org", &identity);
        expect_str(identity.nick, "alice", "identity nick");
        expect_str(identity.user, "user", "identity user");
        expect_str(identity.host, "example.org", "identity host");

        irc_parse_identity("irc.example.org", &identity);
        expect_str(identity.nick, "", "server prefix has no nick");
        expect_str(identity.user, "", "server prefix has no user");
        expect_str(identity.host, "", "server prefix has no host");

        irc_parse_identity(NULL, &identity);
        expect_str(identity.nick, "", "NULL prefix has no nick");
    }

    puts("PASS: IRC parser tests");
    return 0;
}
