#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <tcl.h>

#define TICLE_BUFSIZE 4096

struct ticle_ctx {
    int sock;
    Tcl_Interp *interp;
};

static int send_all(int fd, const char *buf, size_t len)
{
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, buf + off, len - off, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)n;
    }
    return 0;
}

static int irc_send_line(struct ticle_ctx *ctx, const char *line)
{
    size_t len = strlen(line);
    if (send_all(ctx->sock, line, len) < 0 || send_all(ctx->sock, "\r\n", 2) < 0)
        return -1;
    return 0;
}

static int tcl_raw_cmd(ClientData cd, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const objv[])
{
    struct ticle_ctx *ctx = (struct ticle_ctx *)cd;
    const char *line;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "line");
        return TCL_ERROR;
    }

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

static int connect_tcp(const char *host, const char *port)
{
    struct addrinfo hints, *res, *it;
    int fd = -1;
    int rc;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (it = res; it; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

static void dispatch_tcl_line(struct ticle_ctx *ctx, const char *line)
{
    Tcl_CmdInfo info;
    Tcl_Obj *objv[2];

    if (!Tcl_GetCommandInfo(ctx->interp, "ticle::on_line", &info)) return;

    objv[0] = Tcl_NewStringObj("ticle::on_line", -1);
    objv[1] = Tcl_NewStringObj(line, -1);
    Tcl_IncrRefCount(objv[0]);
    Tcl_IncrRefCount(objv[1]);

    if (Tcl_EvalObjv(ctx->interp, 2, objv, TCL_EVAL_GLOBAL) != TCL_OK)
        fprintf(stderr, "Tcl callback error: %s\n", Tcl_GetStringResult(ctx->interp));

    Tcl_DecrRefCount(objv[0]);
    Tcl_DecrRefCount(objv[1]);
}

static void handle_line(struct ticle_ctx *ctx, const char *line)
{
    if (strncmp(line, "PING ", 5) == 0) {
        char pong[TICLE_BUFSIZE];
        if (snprintf(pong, sizeof pong, "PONG %s", line + 5) < (int)sizeof pong)
            (void)irc_send_line(ctx, pong);
    }
    dispatch_tcl_line(ctx, line);
}

static int run_loop(struct ticle_ctx *ctx)
{
    char in[TICLE_BUFSIZE];
    char line[TICLE_BUFSIZE];
    size_t used = 0;

    for (;;) {
        ssize_t n = recv(ctx->sock, in, sizeof in, 0);
        size_t i;

        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            return -1;
        }

        for (i = 0; i < (size_t)n; ++i) {
            char c = in[i];
            if (c == '\n') {
                if (used && line[used - 1] == '\r') --used;
                line[used] = '\0';
                handle_line(ctx, line);
                used = 0;
            } else if (used + 1 < sizeof line) {
                line[used++] = c;
            } else {
                fprintf(stderr, "dropping overlong IRC line\n");
                used = 0;
            }
        }
    }
}

int main(int argc, char **argv)
{
    struct ticle_ctx ctx;
    const char *host, *port, *nick, *script;
    char reg[TICLE_BUFSIZE];

    if (argc != 5) {
        fprintf(stderr, "usage: %s host port nick script.tcl\n", argv[0]);
        return 2;
    }

    host = argv[1];
    port = argv[2];
    nick = argv[3];
    script = argv[4];

    ctx.sock = connect_tcp(host, port);
    if (ctx.sock < 0) {
        fprintf(stderr, "unable to connect to %s:%s\n", host, port);
        return 1;
    }

    Tcl_FindExecutable(argv[0]);
    ctx.interp = Tcl_CreateInterp();
    if (!ctx.interp || Tcl_Init(ctx.interp) != TCL_OK) {
        fprintf(stderr, "unable to initialize Tcl: %s\n",
                ctx.interp ? Tcl_GetStringResult(ctx.interp) : "allocation failure");
        close(ctx.sock);
        return 1;
    }

    Tcl_CreateNamespace(ctx.interp, "ticle", NULL, NULL);
    Tcl_CreateObjCommand(ctx.interp, "ticle::raw", tcl_raw_cmd, &ctx, NULL);

    if (Tcl_EvalFile(ctx.interp, script) != TCL_OK) {
        fprintf(stderr, "script error: %s\n", Tcl_GetStringResult(ctx.interp));
        Tcl_DeleteInterp(ctx.interp);
        Tcl_Finalize();
        close(ctx.sock);
        return 1;
    }

    if (snprintf(reg, sizeof reg, "NICK %s", nick) >= (int)sizeof reg ||
        irc_send_line(&ctx, reg) < 0 ||
        snprintf(reg, sizeof reg, "USER %s 0 * :TiCle IRC bot", nick) >= (int)sizeof reg ||
        irc_send_line(&ctx, reg) < 0) {
        fprintf(stderr, "failed to register on IRC\n");
        Tcl_DeleteInterp(ctx.interp);
        Tcl_Finalize();
        close(ctx.sock);
        return 1;
    }

    (void)run_loop(&ctx);

    Tcl_DeleteInterp(ctx.interp);
    Tcl_Finalize();
    close(ctx.sock);
    return 0;
}
