# inaNutShell — Remote Shell (Phase 2)

A networked layer on top of `inaNutShell`: connect from another machine
(or another terminal) and run a full shell session over a TCP socket.

This folder is **self-contained** — it does not modify the base shell.
The server simply launches the existing `../inaNutShell` binary with its
standard streams (stdin / stdout / stderr) wired to the client socket.
That is exactly the mechanism behind `telnetd`.

## Architecture

```
   client.c                              server.c
   --------                              --------
   connect()  ------- TCP socket ----->   accept()
   keyboard --> socket                       |
   socket   --> screen                    fork()      one process per client
                                             |
                                          dup2(socket -> fd 0, 1, 2)
                                          execl("../inaNutShell")
```

Each client gets its own shell **process**: its own working directory,
environment and history — fully isolated. The server stays in an
`accept` / `fork` loop, so it is **multi-client by design**.

## Build

```sh
cd ..          # build the base shell first
make
cd remote
make
```

## Run

Terminal 1 — start the server (default port 4242):

```sh
./server                              # port 4242, shell = ../inaNutShell
./server 5000                         # custom port
./server 5000 /path/to/inaNutShell    # custom shell binary
```

Terminal 2 — connect a client:

```sh
./client 127.0.0.1 4242
```

Type commands as in a local shell. `exit` or Ctrl+D ends the session.
Open more terminals with more `./client` instances to see multi-client.

## Security — read this

Phase 2 has **no authentication and no encryption**. Anyone able to
reach the port gets a full shell on this machine, and all traffic
(commands *and* output) travels in clear text. Run it **only on
localhost or a trusted LAN**. Authentication and an encrypted channel
are Phase 3.

## Known limits (by design, for now)

- The socket is a raw byte stream, not a terminal (PTY). Line-oriented
  commands work; full-screen / interactive programs (`vim`, `top`, and
  TUI apps) will not render correctly. A PTY is a later stretch goal.
- Ctrl+C in the client is local; it is not forwarded to the remote
  command. Use it after the PTY work.
