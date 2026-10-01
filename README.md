# TiCle

TiCle is a standalone IRC bot written in C with embedded Tcl for scripting.

The core IRC client and bot runtime are implemented in C. Bot behaviour is intended to live primarily in Tcl scripts through a small, stable scripting API.

## Current scope

TiCle currently provides:

- standalone C IRC bot core
- embedded Tcl interpreter
- basic TCP IRC connection and line-oriented event loop
- IRC registration (`NICK` / `USER`)
- automatic `PING` -> `PONG`
- Tcl callback for incoming IRC lines
- Tcl command for sending raw IRC lines
- example Tcl script
- simple build system and GitHub Actions CI

TiCle does **not** require PBMP, BotWeb, BotAI, or BotLogic. Those are optional integrations planned on top of the standalone bot.

## Architecture

```text
IRC network
    |
    v
TiCle C core
    |
    +-- socket / IRC transport
    +-- event loop
    +-- Tcl interpreter
            |
            +-- local Tcl scripts
            +-- optional PBMP adapter
                    |
                    +-- BotAI
                    +-- BotWeb
```

PBMP is intended to be the integration boundary. TiCle should continue normal IRC operation if PBMP, BotWeb, BotAI, or BotLogic are unavailable.

## Build

Requirements:

- C compiler with C11 support
- Tcl development headers and library
- POSIX sockets

On Debian/Ubuntu:

```sh
sudo apt install build-essential tcl-dev
make
```

On Alpine:

```sh
apk add build-base tcl-dev
make
```

## Run

```sh
./ticle irc.example.net 6667 TiCle scripts/example.tcl
```

Configuration-file startup is also supported:

```sh
./ticle -c config/example.conf
```

The M1 configuration format is deliberately small and data-only. Supported keys are `host`, `port`, `nick`, `user`, `realname`, and `script`. The required keys are `host`, `port`, `nick`, and `script`; `user` defaults to the nickname and `realname` defaults to `TiCle IRC bot`. Unknown keys are rejected.

The original CLI remains supported, including an extended identity form:

```sh
./ticle irc.example.net 6667 TiCle scripts/example.tcl ticle "TiCle IRC bot"
```

TLS is not implemented yet. Use a test IRC server or local IRC daemon where plain TCP is acceptable.

## Tcl API

The low-level escape hatch remains available:

```tcl
ticle::raw "PRIVMSG #channel :hello"
```

If the script defines `ticle::on_line`, TiCle calls it for every IRC line received:

```tcl
proc ticle::on_line {line} {
    puts "IRC: $line"
}
```

Structured helpers are available for `PRIVMSG`, `NOTICE`, `JOIN`, `PART`, `NICK`, `MODE`, `KICK`, and `WHOIS`. Incoming lines are parsed into structured message dictionaries and command-specific callbacks. Tcl modules can be loaded, unloaded, and reloaded through the module lifecycle API.

Run the regression suite with:

```sh
make check
```

## Roadmap direction

M1 establishes the structured Tcl API, module lifecycle, parser/config regression tests, IRC identity handling, and startup configuration. M2 focuses on transport reliability: a nonblocking event loop, reconnect/backoff, connection state, nick-collision handling, and outbound flood/rate control. Later milestones cover TLS, IRCv3, stronger Tcl privilege boundaries, OCI/operations, and optional PBMP integration.

## License

Software is intended to use the MIT License unless otherwise documented.
