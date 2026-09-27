#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <tcl.h>

#include "irc.h"
#include "config.h"
#include "state.h"
#include "backoff.h"
#include "nick.h"

#define TICLE_BUFSIZE TICLE_IRC_BUFSIZE
#define TICLE_REGISTRATION_TIMEOUT_SECONDS 30

static volatile sig_atomic_t ticle_stop_requested = 0;

static void ticle_signal_handler(int signo) {
    (void)signo;
    ticle_stop_requested = 1;
}

static int wait_seconds_interruptible(unsigned int seconds) {
    struct timespec step = {0, 100000000L};
    unsigned int ticks = seconds * 10U;
    unsigned int i;
    for (i = 0; i < ticks && !ticle_stop_requested; ++i) {
        struct timespec remaining = step;
        while (nanosleep(&remaining, &remaining) < 0) {
            if (errno != EINTR) return -1;
            if (ticle_stop_requested) return 0;
        }
    }
    return 0;
}

struct ticle_ctx {
    int sock;
    Tcl_Interp *interp;
    enum ticle_connection_state state;
    const char *base_nick;
    unsigned int nick_collision;
    time_t registration_started;
};

static int send_all(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, buf + off, len - off, 0);
        if (n < 0) {
            struct pollfd pfd;
            int ready;
            if (errno == EINTR) continue;
            if (errno != EAGAIN && errno != EWOULDBLOCK) return -1;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            do {
                ready = poll(&pfd, 1, 10000);
            } while (ready < 0 && errno == EINTR);
            if (ready <= 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
                return -1;
            continue;
        }
        if (n == 0) return -1;
        off += (size_t)n;
    }
    return 0;
}

static int irc_send_line(struct ticle_ctx *ctx, const char *line) {
    return send_all(ctx->sock, line, strlen(line)) < 0 ||
           send_all(ctx->sock, "\r\n", 2) < 0 ? -1 : 0;
}

static int tcl_raw_cmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    struct ticle_ctx *ctx = (struct ticle_ctx *)cd;
    const char *line;
    if (objc != 2) { Tcl_WrongNumArgs(interp, 1, objv, "line"); return TCL_ERROR; }
    line = Tcl_GetString(objv[1]);
    if (strchr(line, '\r') || strchr(line, '\n')) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("IRC line must not contain CR/LF", -1));
        return TCL_ERROR;
    }
    if (irc_send_line(ctx, line) < 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("failed to send IRC line", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int connect_tcp(const char *host, const char *port) {
    struct addrinfo hints, *res, *it;
    int fd = -1, rc;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (it = res; it; it = it->ai_next) {
        int flags, err = 0;
        socklen_t errlen = sizeof err;
        struct pollfd pfd;

        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;

        flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(fd);
            fd = -1;
            continue;
        }

        rc = connect(fd, it->ai_addr, it->ai_addrlen);
        if (rc == 0) break;
        if (errno != EINPROGRESS) {
            close(fd);
            fd = -1;
            continue;
        }

        pfd.fd = fd;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        do {
            rc = poll(&pfd, 1, 10000);
        } while (rc < 0 && errno == EINTR);

        if (rc <= 0 || !(pfd.revents & (POLLOUT | POLLERR | POLLHUP)) ||
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0 || err != 0) {
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(res);
    return fd;
}

static void eval_callback(struct ticle_ctx *ctx, int objc, Tcl_Obj **objv) {
    int i;
    for (i = 0; i < objc; ++i) Tcl_IncrRefCount(objv[i]);
    if (Tcl_EvalObjv(ctx->interp, objc, objv, TCL_EVAL_GLOBAL) != TCL_OK)
        fprintf(stderr, "Tcl callback error: %s\n", Tcl_GetStringResult(ctx->interp));
    for (i = 0; i < objc; ++i) Tcl_DecrRefCount(objv[i]);
}

static void dispatch_tcl_line(struct ticle_ctx *ctx, const char *line) {
    Tcl_CmdInfo info;
    Tcl_Obj *objv[2];
    if (!Tcl_GetCommandInfo(ctx->interp, "ticle::on_line", &info)) return;
    objv[0] = Tcl_NewStringObj("ticle::on_line", -1);
    objv[1] = Tcl_NewStringObj(line, -1);
    eval_callback(ctx, 2, objv);
}

static Tcl_Obj *message_dict(struct ticle_ctx *ctx, const struct irc_message *msg) {
    Tcl_Obj *dict = Tcl_NewDictObj();
    Tcl_Obj *params = Tcl_NewListObj(0, NULL);
    struct irc_identity identity;
    int i;

    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("prefix", -1),
                   Tcl_NewStringObj(msg->prefix ? msg->prefix : "", -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("command", -1),
                   Tcl_NewStringObj(msg->command, -1));

    for (i = 0; i < msg->nparams; ++i)
        Tcl_ListObjAppendElement(ctx->interp, params, Tcl_NewStringObj(msg->params[i], -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("params", -1), params);

    irc_parse_identity(msg->prefix, &identity);

    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("nick", -1),
                   Tcl_NewStringObj(identity.nick, -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("user", -1),
                   Tcl_NewStringObj(identity.user, -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("host", -1),
                   Tcl_NewStringObj(identity.host, -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("target", -1),
                   Tcl_NewStringObj(msg->nparams > 0 ? msg->params[0] : "", -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("text", -1),
                   Tcl_NewStringObj(msg->nparams > 1 ? msg->params[msg->nparams - 1] : "", -1));
    return dict;
}

static void dispatch_tcl_message(struct ticle_ctx *ctx, const struct irc_message *msg) {
    Tcl_CmdInfo info;
    Tcl_Obj *objv[2];
    if (!Tcl_GetCommandInfo(ctx->interp, "ticle::on_message", &info)) return;
    objv[0] = Tcl_NewStringObj("ticle::on_message", -1);
    objv[1] = message_dict(ctx, msg);
    eval_callback(ctx, 2, objv);
}

static void dispatch_tcl_typed(struct ticle_ctx *ctx, const struct irc_message *msg) {
    Tcl_CmdInfo info;
    Tcl_Obj *objv[2];
    char callback[64];
    size_t i, n, base = strlen("ticle::on_");

    n = strlen(msg->command);
    if (base + n + 1 > sizeof callback) return;
    strcpy(callback, "ticle::on_");
    for (i = 0; i < n; ++i) {
        char ch = msg->command[i];
        callback[base + i] = (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
    }
    callback[base + n] = '\0';

    if (!Tcl_GetCommandInfo(ctx->interp, callback, &info)) return;
    objv[0] = Tcl_NewStringObj(callback, -1);
    objv[1] = message_dict(ctx, msg);
    eval_callback(ctx, 2, objv);
}

static void handle_line(struct ticle_ctx *ctx, const char *line) {
    struct irc_message msg;
    if (strncmp(line, "PING ", 5) == 0) {
        char pong[TICLE_BUFSIZE];
        if (snprintf(pong, sizeof pong, "PONG %s", line + 5) < (int)sizeof pong)
            (void)irc_send_line(ctx, pong);
    }
    dispatch_tcl_line(ctx, line);
    if (irc_parse_message(line, &msg) == 0) {
        if (ctx->state == TICLE_REGISTERING && strcmp(msg.command, "001") == 0) {
            ctx->state = TICLE_ONLINE;
            ctx->nick_collision = 0;
            ctx->registration_started = (time_t)0;
        } else if (ctx->state == TICLE_REGISTERING && strcmp(msg.command, "433") == 0) {
            char candidate[TICLE_BUFSIZE];
            char command[TICLE_BUFSIZE];
            ++ctx->nick_collision;
            if (ticle_nick_candidate(candidate, sizeof candidate,
                                     ctx->base_nick, ctx->nick_collision) == 0 &&
                snprintf(command, sizeof command, "NICK %s", candidate) < (int)sizeof command) {
                (void)irc_send_line(ctx, command);
            }
        }
        dispatch_tcl_message(ctx, &msg);
        dispatch_tcl_typed(ctx, &msg);
    }
}

static int run_loop(struct ticle_ctx *ctx) {
    char in[TICLE_BUFSIZE], line[TICLE_BUFSIZE];
    size_t used = 0;
    struct pollfd pfd;

    pfd.fd = ctx->sock;
    pfd.events = POLLIN;
    pfd.revents = 0;

    for (;;) {
        int ready = poll(&pfd, 1, 1000);
        if (ticle_stop_requested) return 0;
        if (ctx->state == TICLE_REGISTERING &&
            ctx->registration_started != (time_t)0 &&
            time(NULL) - ctx->registration_started >= TICLE_REGISTRATION_TIMEOUT_SECONDS) {
            fprintf(stderr, "IRC registration timed out after %d seconds\n",
                    TICLE_REGISTRATION_TIMEOUT_SECONDS);
            return -1;
        }
        if (ready == 0) continue;
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            return -1;
        }

        if (pfd.revents & (POLLERR | POLLNVAL)) {
            fprintf(stderr, "IRC socket poll error\n");
            return -1;
        }
        if (pfd.revents & POLLHUP) {
            return 0;
        }
        if (!(pfd.revents & POLLIN)) continue;

        for (;;) {
            ssize_t n = recv(ctx->sock, in, sizeof in, 0);
            size_t i;
            if (n == 0) return 0;
            if (n < 0) {
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                perror("recv");
                return -1;
            }
            for (i = 0; i < (size_t)n; ++i) {
                char ch = in[i];
                if (ch == '\n') {
                    if (used && line[used - 1] == '\r') --used;
                    line[used] = '\0';
                    handle_line(ctx, line);
                    used = 0;
                } else if (used + 1 < sizeof line) {
                    line[used++] = ch;
                } else {
                    fprintf(stderr, "dropping overlong IRC line\n");
                    used = 0;
                }
            }
            break;
        }
    }
}

int main(int argc, char **argv) {
    struct ticle_ctx ctx;
    struct ticle_config config;
    const char *host, *port, *nick, *user, *realname, *script;
    char nick_line[TICLE_BUFSIZE], user_line[TICLE_BUFSIZE];
    if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        if (ticle_config_load(argv[2], &config) < 0) {
            fprintf(stderr, "unable to load config: %s\n", argv[2]);
            return 2;
        }
        host = config.host; port = config.port; nick = config.nick;
        user = config.user; realname = config.realname; script = config.script;
    } else if (argc == 5 || argc == 7) {
        host = argv[1]; port = argv[2]; nick = argv[3]; script = argv[4];
        user = argc == 7 ? argv[5] : nick;
        realname = argc == 7 ? argv[6] : "TiCle IRC bot";
    } else {
        fprintf(stderr, "usage: %s -c config | host port nick script.tcl [user realname]\n", argv[0]);
        return 2;
    }
    signal(SIGINT, ticle_signal_handler);
    signal(SIGTERM, ticle_signal_handler);

    ctx.state = TICLE_DISCONNECTED;
    ctx.sock = -1;
    ctx.base_nick = nick;
    ctx.nick_collision = 0;
    ctx.registration_started = (time_t)0;

    Tcl_FindExecutable(argv[0]);
    ctx.interp = Tcl_CreateInterp();
    if (!ctx.interp || Tcl_Init(ctx.interp) != TCL_OK) {
        fprintf(stderr, "unable to initialize Tcl: %s\n",
                ctx.interp ? Tcl_GetStringResult(ctx.interp) : "allocation failure");
        return 1;
    }
    Tcl_CreateNamespace(ctx.interp, "ticle", NULL, NULL);
    Tcl_CreateObjCommand(ctx.interp, "ticle::raw", tcl_raw_cmd, &ctx, NULL);
    if (Tcl_EvalFile(ctx.interp, script) != TCL_OK) {
        fprintf(stderr, "script error: %s\n", Tcl_GetStringResult(ctx.interp));
        Tcl_DeleteInterp(ctx.interp);
        Tcl_Finalize();
        return 1;
    }

    {
        unsigned int attempt = 0;
        while (!ticle_stop_requested) {
            unsigned int delay;

            ctx.state = TICLE_CONNECTING;
            ctx.sock = connect_tcp(host, port);
            if (ctx.sock < 0) {
                ctx.state = TICLE_DISCONNECTED;
                delay = ticle_backoff_seconds(attempt++);
                fprintf(stderr, "unable to connect to %s:%s; retrying in %u seconds\n",
                        host, port, delay);
                wait_seconds_interruptible(delay);
                continue;
            }

            ctx.state = TICLE_REGISTERING;
            ctx.nick_collision = 0;
            ctx.registration_started = time(NULL);
            if (irc_format_registration(nick_line, sizeof nick_line,
                                        user_line, sizeof user_line,
                                        nick, user, realname) < 0 ||
                irc_send_line(&ctx, nick_line) < 0 ||
                irc_send_line(&ctx, user_line) < 0) {
                fprintf(stderr, "failed to register on IRC\n");
                close(ctx.sock);
                ctx.sock = -1;
                ctx.state = TICLE_DISCONNECTED;
                delay = ticle_backoff_seconds(attempt++);
                wait_seconds_interruptible(delay);
                continue;
            }

            attempt = 0;
            (void)run_loop(&ctx);
            close(ctx.sock);
            ctx.sock = -1;
            ctx.state = TICLE_DISCONNECTED;
            if (ticle_stop_requested) break;

            delay = ticle_backoff_seconds(attempt++);
            fprintf(stderr, "IRC connection lost; reconnecting in %u seconds\n", delay);
            wait_seconds_interruptible(delay);
        }
    }

    ctx.state = TICLE_STOPPING;
    Tcl_DeleteInterp(ctx.interp);
    Tcl_Finalize();
    return 0;
}
