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
#define TICLE_MAX_PARAMS 15

struct ticle_ctx { int sock; Tcl_Interp *interp; };

struct irc_message {
    char storage[TICLE_BUFSIZE];
    char *prefix;
    char *command;
    char *params[TICLE_MAX_PARAMS];
    int nparams;
};

static int send_all(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, buf + off, len - off, 0);
        if (n < 0) { if (errno == EINTR) continue; return -1; }
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

static int parse_irc_message(const char *line, struct irc_message *msg) {
    char *p, *space;
    memset(msg, 0, sizeof *msg);
    if (strlen(line) >= sizeof msg->storage) return -1;
    strcpy(msg->storage, line);
    p = msg->storage;

    if (*p == ':') {
        msg->prefix = ++p;
        space = strchr(p, ' ');
        if (!space) return -1;
        *space = '\0';
        p = space + 1;
        while (*p == ' ') ++p;
    }

    if (!*p) return -1;
    msg->command = p;
    space = strchr(p, ' ');
    if (!space) return 0;
    *space = '\0';
    p = space + 1;

    while (*p && msg->nparams < TICLE_MAX_PARAMS) {
        while (*p == ' ') ++p;
        if (!*p) break;
        if (*p == ':') {
            msg->params[msg->nparams++] = p + 1;
            break;
        }
        msg->params[msg->nparams++] = p;
        space = strchr(p, ' ');
        if (!space) break;
        *space = '\0';
        p = space + 1;
    }
    return 0;
}

static int connect_tcp(const char *host, const char *port) {
    struct addrinfo hints, *res, *it;
    int fd = -1, rc;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc)); return -1; }
    for (it = res; it; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) break;
        close(fd); fd = -1;
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

static void dispatch_tcl_message(struct ticle_ctx *ctx, const struct irc_message *msg) {
    Tcl_CmdInfo info;
    Tcl_Obj *objv[2], *dict, *params;
    int i;
    if (!Tcl_GetCommandInfo(ctx->interp, "ticle::on_message", &info)) return;

    dict = Tcl_NewDictObj();
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("prefix", -1),
                   Tcl_NewStringObj(msg->prefix ? msg->prefix : "", -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("command", -1),
                   Tcl_NewStringObj(msg->command, -1));
    params = Tcl_NewListObj(0, NULL);
    for (i = 0; i < msg->nparams; ++i)
        Tcl_ListObjAppendElement(ctx->interp, params, Tcl_NewStringObj(msg->params[i], -1));
    Tcl_DictObjPut(ctx->interp, dict, Tcl_NewStringObj("params", -1), params);

    objv[0] = Tcl_NewStringObj("ticle::on_message", -1);
    objv[1] = dict;
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
    if (parse_irc_message(line, &msg) == 0)
        dispatch_tcl_message(ctx, &msg);
}

static int run_loop(struct ticle_ctx *ctx) {
    char in[TICLE_BUFSIZE], line[TICLE_BUFSIZE];
    size_t used = 0;
    for (;;) {
        ssize_t n = recv(ctx->sock, in, sizeof in, 0);
        size_t i;
        if (n == 0) return 0;
        if (n < 0) { if (errno == EINTR) continue; perror("recv"); return -1; }
        for (i = 0; i < (size_t)n; ++i) {
            char c = in[i];
            if (c == '\n') {
                if (used && line[used - 1] == '\r') --used;
                line[used] = '\0'; handle_line(ctx, line); used = 0;
            } else if (used + 1 < sizeof line) line[used++] = c;
            else { fprintf(stderr, "dropping overlong IRC line\n"); used = 0; }
        }
    }
}

int main(int argc, char **argv) {
    struct ticle_ctx ctx;
    const char *host, *port, *nick, *script;
    char reg[TICLE_BUFSIZE];
    if (argc != 5) { fprintf(stderr, "usage: %s host port nick script.tcl\n", argv[0]); return 2; }
    host = argv[1]; port = argv[2]; nick = argv[3]; script = argv[4];
    ctx.sock = connect_tcp(host, port);
    if (ctx.sock < 0) { fprintf(stderr, "unable to connect to %s:%s\n", host, port); return 1; }

    Tcl_FindExecutable(argv[0]);
    ctx.interp = Tcl_CreateInterp();
    if (!ctx.interp || Tcl_Init(ctx.interp) != TCL_OK) {
        fprintf(stderr, "unable to initialize Tcl: %s\n",
                ctx.interp ? Tcl_GetStringResult(ctx.interp) : "allocation failure");
        close(ctx.sock); return 1;
    }
    Tcl_CreateNamespace(ctx.interp, "ticle", NULL, NULL);
    Tcl_CreateObjCommand(ctx.interp, "ticle::raw", tcl_raw_cmd, &ctx, NULL);
    if (Tcl_EvalFile(ctx.interp, script) != TCL_OK) {
        fprintf(stderr, "script error: %s\n", Tcl_GetStringResult(ctx.interp));
        Tcl_DeleteInterp(ctx.interp); Tcl_Finalize(); close(ctx.sock); return 1;
    }
    if (snprintf(reg, sizeof reg, "NICK %s", nick) >= (int)sizeof reg ||
        irc_send_line(&ctx, reg) < 0 ||
        snprintf(reg, sizeof reg, "USER %s 0 * :TiCle IRC bot", nick) >= (int)sizeof reg ||
        irc_send_line(&ctx, reg) < 0) {
        fprintf(stderr, "failed to register on IRC\n");
        Tcl_DeleteInterp(ctx.interp); Tcl_Finalize(); close(ctx.sock); return 1;
    }
    (void)run_loop(&ctx);
    Tcl_DeleteInterp(ctx.interp); Tcl_Finalize(); close(ctx.sock);
    return 0;
}
