# inaNutShell

`inaNutShell` is a small interactive Unix shell written in C, built for the
S8 System Administration project (a mini-shell in C). It covers command
execution, built-in commands, I/O redirections, pipes, background jobs,
signal handling, and several bonus features.

## Build

```sh
make clean && make
```

Requires `gcc`. The shell uses Unix process APIs (`fork`, `execvp`, `pipe`,
`sigaction`, ...), so it must be built on Linux — natively or under WSL.
It does not compile with native Windows toolchains.

## Run

```sh
./inaNutShell
```

## Implemented Features

### Mandatory

- Interactive prompt: `inaNutShell> `
- External command execution with `fork()` and `execvp()`
- Built-ins, run without forking: `cd [path]`, `exit [code]`
- Error handling: unknown commands, missing files, invalid syntax
- I/O redirections: input `<`, output `>`, append `>>`
- Pipelines of arbitrary length with `|`
- Background execution with a trailing `&`
- Signal handling:
  - `SIGINT` (Ctrl+C) interrupts the running command, not the shell
  - `SIGCHLD` reaps finished children so no zombies are left behind
  - `SIGQUIT` (Ctrl+\) is ignored by the shell

### Bonus

- Command history, listed with the `history` built-in
- Re-execution of a past command with `!n`
- Environment variable expansion, e.g. `echo $HOME`
- Several commands on one line, separated by `;`

## Project Layout

| File         | Role                                                  |
|--------------|-------------------------------------------------------|
| `main.c`     | REPL loop, history (`!n`) expansion, `;` splitting    |
| `parser.c`   | Tokenizer: builds a `Pipeline` of `Command` structs   |
| `executor.c` | Process creation, redirections, pipes                 |
| `builtins.c` | Built-in commands and the command-history buffer      |
| `signals.c`  | Signal handlers (`SIGINT`, `SIGCHLD`, `SIGQUIT`)      |
| `utils.c`    | Helpers: prompt, line reading, string utilities       |

## Validation

```sh
make clean && make
valgrind --leak-check=full --track-fds=yes ./inaNutShell
```

`tests.txt` lists representative commands to exercise every stage.
