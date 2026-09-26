# TiCle

TiCle is a standalone IRC bot written in C with embedded Tcl for scripting.

The core IRC client and bot runtime are implemented in C. Bot behaviour is intended to live primarily in Tcl scripts through a small, stable scripting API.

## M0 scope

M0 establishes the project skeleton and architecture:

- standalone C IRC bot core
- embedded Tcl interpreter
- basic TCP IRC connection and line-oriented event loop
- IRC registration (`NICK` / `USER`)
- automatic `PING` -> `PONG`
- Tcl callback for incoming IRC lines
- Tcl command for sending raw IRC lines
- example Tcl script
- simple build system and GitHub Actions CI

TiCle does **not** require PBMP, BotAI, or BotWeb. Those are optional integrations planned on top of the standalone bot.

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

PBMP is intended to be the integration boundary. TiCle should continue normal IRC operation if PBMP, BotAI, or BotWeb are unavailable.

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

For M0, TLS is not implemented yet. Use a test IRC server or local IRC daemon where plain TCP is acceptable.

## Tcl API (M0)

TiCle registers this command:

```tcl
ticle::raw "PRIVMSG #channel :hello"
```

If the script defines `ticle::on_line`, TiCle calls it for every IRC line received:

```tcl
proc ticle::on_line {line} {
    puts "IRC: $line"
}
```

The Tcl API will grow toward structured IRC events rather than requiring scripts to parse raw protocol lines themselves.

## Roadmap direction

After M0, likely milestones include structured IRC parsing/events, reconnect/backoff, configuration, TLS, IRCv3 capability negotiation, channel management, timers, modular Tcl loading, privilege/sandbox controls, tests, packaging/OCI, and optional PBMP integration.

## License

Software is intended to use the MIT License unless otherwise documented.
