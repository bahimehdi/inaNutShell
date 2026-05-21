/* server.c — inaNutShell remote shell server (Phase 2)
 *
 * Listens on a TCP port. For each client that connects, it forks a child
 * process, wires the client socket onto the child's standard streams
 * (stdin/stdout/stderr), and replaces the child with the existing
 * inaNutShell binary. The whole shell then runs over the network.
 *
 * This is the same mechanism as telnetd. The base shell is not modified:
 * the socket simply becomes its descriptors 0, 1 and 2.
 *
 * WARNING: Phase 2 has no authentication and no encryption. Run it only
 * on localhost or a trusted LAN.
 */

#include <errno.h>       /* errno, EINTR — error codes set by failed system calls */
#include <stdio.h>       /* printf, perror — console messages and error reporting */
#include <stdlib.h>      /* atoi — convert a text argument into an integer */
#include <string.h>      /* memset — fill a block of memory with a chosen byte */
#include <signal.h>      /* sigaction, SIGCHLD — installing a signal handler */
#include <unistd.h>      /* fork, close, dup2, execl, _exit, STDIN_FILENO ... */
#include <sys/types.h>   /* pid_t — the integer type that holds a process id */
#include <sys/socket.h>  /* socket, bind, listen, accept, setsockopt */
#include <sys/wait.h>    /* waitpid, WNOHANG — collecting finished child processes */
#include <netinet/in.h>  /* struct sockaddr_in, htons, htonl, INADDR_ANY */
#include <arpa/inet.h>   /* inet_ntoa, ntohs — address and byte-order conversions */

#define DEFAULT_PORT 4242              /* TCP port used when none is given on the command line */
#define DEFAULT_SHELL "../inaNutShell" /* path to the shell binary the server launches */
#define BACKLOG 10                     /* how many pending connections may queue before accept() */

/* reap_children: SIGCHLD handler. The kernel runs it automatically whenever
   a child (one client session) ends, so the dead child is collected at once
   and never lingers as a zombie process. */
static void reap_children(int sig)  /* 'sig' is the signal number — always SIGCHLD here */
{
    int saved_errno = errno;  /* save errno: the handler must not corrupt the value the main code relies on */
    (void)sig;  /* explicitly ignore 'sig' — this silences the -Wextra "unused parameter" warning */
    while (waitpid(-1, NULL, WNOHANG) > 0)  /* collect every finished child: -1 = any child, WNOHANG = never block */
        ;  /* empty loop body — all the work is done by the waitpid() call in the condition */
    errno = saved_errno;  /* restore errno to whatever it was before this handler interrupted the program */
}

/* main: builds the listening socket, then loops forever accepting clients. */
int main(int argc, char *argv[])  /* argc = number of arguments, argv = the argument strings */
{
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;           /* 1st argument = port, else the default */
    const char *shell_path = (argc > 2) ? argv[2] : DEFAULT_SHELL;  /* 2nd argument = shell path, else the default */
    int listen_fd;             /* descriptor of the socket that waits for new connections */
    int opt = 1;               /* the value "1" (meaning "enabled"), handed to setsockopt() below */
    struct sockaddr_in addr;   /* holds the server's own address (IP + port) for bind() */
    struct sigaction sa;       /* describes how the SIGCHLD signal should be handled */

    sa.sa_handler = reap_children;            /* when SIGCHLD arrives, the kernel must call reap_children() */
    sigemptyset(&sa.sa_mask);                 /* block no additional signals while the handler is running */
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;  /* SA_RESTART: auto-resume interrupted syscalls; SA_NOCLDSTOP: ignore merely-stopped children */
    sigaction(SIGCHLD, &sa, NULL);            /* install this handler for the SIGCHLD signal */

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);  /* create a socket: AF_INET = IPv4, SOCK_STREAM = TCP */
    if (listen_fd < 0) {  /* socket() returns -1 when it fails */
        perror("socket");  /* print the reason it failed */
        return 1;          /* abort — nothing can work without the socket */
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  /* SO_REUSEADDR: let the server re-bind the port immediately after a restart */

    memset(&addr, 0, sizeof(addr));             /* zero every byte of the address struct so no field stays uninitialised */
    addr.sin_family = AF_INET;                  /* declare this as an IPv4 address */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* INADDR_ANY = accept on every network interface; htonl = host-to-network byte order */
    addr.sin_port = htons((unsigned short)port);/* set the listening port, converted to network byte order with htons */

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {  /* attach the socket to that IP and port */
        perror("bind");      /* report the failure (a common cause: the port is already in use) */
        close(listen_fd);    /* release the socket descriptor */
        return 1;            /* abort */
    }
    if (listen(listen_fd, BACKLOG) < 0) {  /* switch the socket into listening mode so it can accept connections */
        perror("listen");    /* report the failure */
        close(listen_fd);    /* release the socket descriptor */
        return 1;            /* abort */
    }
    printf("inaNutShell remote server: listening on port %d\n", port);  /* tell the operator the server is up */
    printf("shell binary: %s\n", shell_path);  /* tell the operator which shell binary will be launched */
    fflush(stdout);  /* force those two messages out now, in case stdout is buffered */

    for (;;) {  /* infinite loop: each iteration accepts and serves one more client */
        struct sockaddr_in peer;            /* will be filled with the connecting client's address */
        socklen_t peer_len = sizeof(peer);  /* size of 'peer'; accept() both reads and updates this */
        pid_t pid;                          /* will hold the value returned by fork() */
        int client_fd;                      /* descriptor of the socket connected to this one client */

        client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);  /* wait for a client; returns a NEW socket dedicated to it */
        if (client_fd < 0) {       /* accept() returned -1 */
            if (errno == EINTR)    /* the -1 was caused only by a signal (e.g. SIGCHLD) interrupting the wait */
                continue;          /* not a real error — just loop back and wait again */
            perror("accept");      /* a genuine accept() failure */
            continue;              /* skip this round but keep the server alive */
        }
        printf("[+] client connected: %s:%d\n",                  /* log the new connection ... */
               inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));  /* ... showing the client's IP (as text) and port (host byte order) */
        fflush(stdout);  /* force the log line out immediately */

        pid = fork();  /* split into two processes: a child to serve the client, the parent to keep listening */
        if (pid < 0) {         /* fork() failed (returns -1) */
            perror("fork");    /* report it */
            close(client_fd);  /* drop this client since it cannot be served */
            continue;          /* keep the server running */
        }
        if (pid == 0) {  /* fork() returns 0 inside the child — this block is the child process */
            close(listen_fd);  /* the child never accepts new clients, so it closes the listening socket */
            if (dup2(client_fd, STDIN_FILENO) < 0       /* point fd 0 (stdin) at the socket: the shell reads commands FROM the client */
                || dup2(client_fd, STDOUT_FILENO) < 0   /* point fd 1 (stdout) at the socket: the shell's output goes TO the client */
                || dup2(client_fd, STDERR_FILENO) < 0) {/* point fd 2 (stderr) at the socket: error messages go to the client too */
                perror("dup2");  /* one of the three redirections failed */
                _exit(1);        /* _exit (not exit) in a child: quit immediately without flushing inherited buffers */
            }
            close(client_fd);  /* the original descriptor is now redundant — fds 0/1/2 already point at the socket */
            execl(shell_path, "inaNutShell", (char *)NULL);  /* replace this process with the shell; argv[0]="inaNutShell", no extra arguments */
            perror("execl");  /* reached ONLY if execl failed — on success this process has already become the shell */
            _exit(1);         /* quit the child whose exec failed */
        }
        close(client_fd);  /* parent: it does not talk to the client (the child does), so it closes its copy */
    }
    return 0;  /* never reached — the for(;;) loop never ends — but required by the int return type */
}
