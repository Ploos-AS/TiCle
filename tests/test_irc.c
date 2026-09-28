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

    expect(irc_parse_message("@time=2026-09-28T02:00:00Z;account=alice :alice!u@example PRIVMSG #test :tagged", &m) == 0, "IRCv3 tags parse");
    expect(m.ntags == 2, "IRCv3 tag count");
    expect_str(m.tags[0].key, "time", "IRCv3 time key");
    expect_str(m.tags[0].value, "2026-09-28T02:00:00Z", "IRCv3 time value");
    expect_str(m.tags[1].key, "account", "IRCv3 account key");
    expect_str(m.tags[1].value, "alice", "IRCv3 account value");
    expect_str(m.command, "PRIVMSG", "tagged command");
    expect_str(m.params[1], "tagged", "tagged trailing text");

    expect(irc_parse_message("@draft/flag PING :token", &m) == 0, "valueless IRCv3 tag parses");
    expect(m.ntags == 1, "valueless tag count");
    expect_str(m.tags[0].key, "draft/flag", "valueless tag key");
    expect(m.tags[0].value == NULL, "valueless tag has NULL value");

    expect(irc_parse_message("@=bad PING :token", &m) == -1, "empty IRCv3 tag key rejected");

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

        {
            char huge[TICLE_IRC_BUFSIZE];
            memset(huge, 'x', sizeof huge - 1);
            huge[sizeof huge - 1] = '\0';
            {
                char prefix[TICLE_IRC_BUFSIZE];
                snprintf(prefix, sizeof prefix, "%s!u@h", huge);
                irc_parse_identity(prefix, &identity);
                expect_str(identity.nick, "", "oversized identity nick rejected");
            }
        }
    }

    {
        char nick_line[128], user_line[256];
        expect(irc_format_registration(nick_line, sizeof nick_line,
                                       user_line, sizeof user_line,
                                       "TiCle", "ticle", "TiCle IRC bot") == 0,
               "registration formats");
        expect_str(nick_line, "NICK TiCle", "registration NICK line");
        expect_str(user_line, "USER ticle 0 * :TiCle IRC bot", "registration USER line");

        expect(irc_format_registration(nick_line, sizeof nick_line,
                                       user_line, sizeof user_line,
                                       "bad nick", "ticle", "TiCle") == -1,
               "registration rejects nick whitespace");
        expect(irc_format_registration(nick_line, sizeof nick_line,
                                       user_line, sizeof user_line,
                                       "TiCle", "bad\nuser", "TiCle") == -1,
               "registration rejects user injection");
        expect(irc_format_registration(nick_line, sizeof nick_line,
                                       user_line, sizeof user_line,
                                       "TiCle", "ticle", "bad\r\nQUIT") == -1,
               "registration rejects realname injection");
        expect(irc_format_registration(nick_line, 5,
                                       user_line, sizeof user_line,
                                       "TiCle", "ticle", "TiCle") == -1,
               "registration rejects truncated output");
    }

    puts("PASS: IRC parser tests");
    return 0;
}
