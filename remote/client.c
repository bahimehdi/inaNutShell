/* client.c — inaNutShell remote shell client (Phase 2)
 *
 * Connects to the remote shell server and relays data in both directions
 * at once:
 *   - the main thread copies the keyboard (stdin) to the socket;
 *   - a second thread copies the socket to the screen (stdout).
 *
 * Two threads are needed because both read() calls are blocking: a single
 * thread stuck waiting on the keyboard would never display incoming output.
 * This is the same pattern as the course's peer.c.
 */

#include <stdio.h>       /* printf, fprintf, perror — output and error reporting */
#include <stdlib.h>      /* atoi, exit — convert text to int, and end the process */
#include <string.h>      /* memset — fill a block of memory with a chosen byte */
#include <unistd.h>      /* read, write, close, STDIN_FILENO, STDOUT_FILENO */
#include <pthread.h>     /* pthread_create, pthread_join — POSIX threads */
#include <sys/socket.h>  /* socket, connect, shutdown, SHUT_WR */
#include <netinet/in.h>  /* struct sockaddr_in, htons */
#include <arpa/inet.h>   /* inet_pton — convert a text IP address into binary form */

#define BUF 4096  /* size, in bytes, of the buffers used to relay data */

/* socket_to_screen: the reader thread. It continuously copies whatever the
   server sends onto our terminal, and ends the whole program once the
   server closes the connection. */
static void *socket_to_screen(void *arg)  /* 'arg' carries the socket descriptor, passed as a generic void pointer */
{
    int sock = *(int *)arg;  /* cast 'arg' back to int* and read the socket descriptor out of it */
    char buf[BUF];           /* buffer holding one chunk of data received from the server */
    ssize_t n;               /* number of bytes returned by read() — ssize_t because it can also be -1 */

    while ((n = read(sock, buf, sizeof(buf))) > 0) {  /* read from the socket; keep looping while at least one byte arrives */
        if (write(STDOUT_FILENO, buf, (size_t)n) != n)  /* copy exactly those n bytes onto the screen */
            break;  /* stop if the screen write was short (an error occurred) */
    }
    printf("\n[connection closed by server]\n");  /* read() returned 0 or -1: the session is over — tell the user */
    exit(0);     /* end the WHOLE program, which also stops the main thread even if it is blocked on the keyboard */
    return NULL; /* never reached — present only so the function matches its void* return type */
}

/* main: connects to the server, starts the reader thread, then relays the
   keyboard to the socket until end-of-input. */
int main(int argc, char *argv[])  /* argc = number of arguments, argv = the argument strings */
{
    int sock;                 /* descriptor of the socket connected to the server */
    struct sockaddr_in addr;  /* holds the server's address (IP + port) we will connect to */
    pthread_t tid;            /* identifier of the reader thread created below */
    char buf[BUF];            /* buffer holding one chunk of keyboard input */
    ssize_t n;                /* number of bytes returned by each keyboard read() */

    if (argc != 3) {  /* the program needs exactly two arguments: a server IP and a port */
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);  /* show the correct way to call it */
        return 1;  /* abort with a non-zero (failure) exit code */
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);  /* create a socket: AF_INET = IPv4, SOCK_STREAM = TCP */
    if (sock < 0) {  /* socket() returns -1 on failure */
        perror("socket");  /* report why it failed */
        return 1;          /* abort */
    }

    memset(&addr, 0, sizeof(addr));  /* zero every byte of the address struct so no field stays uninitialised */
    addr.sin_family = AF_INET;       /* declare this as an IPv4 address */
    addr.sin_port = htons((unsigned short)atoi(argv[2]));  /* convert the port argument to a number, then to network byte order */
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {  /* convert the text IP (argv[1]) to binary; a return of 1 means success */
        fprintf(stderr, "invalid IP address: %s\n", argv[1]);  /* the IP string was malformed */
        close(sock);  /* release the socket */
        return 1;     /* abort */
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {  /* open the TCP connection to the server */
        perror("connect");  /* report the failure (commonly: no server there, or connection refused) */
        close(sock);        /* release the socket */
        return 1;           /* abort */
    }

    if (pthread_create(&tid, NULL, socket_to_screen, &sock) != 0) {  /* start the reader thread; it runs socket_to_screen(&sock) */
        perror("pthread_create");  /* report the failure */
        close(sock);               /* release the socket */
        return 1;                  /* abort */
    }

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {  /* read the keyboard; keep looping while at least one byte is typed */
        if (write(sock, buf, (size_t)n) != n) {  /* forward exactly those n bytes to the server */
            perror("write");  /* the send failed */
            break;            /* stop relaying the keyboard */
        }
    }

    shutdown(sock, SHUT_WR);  /* read() hit end-of-input (Ctrl+D): close only our SENDING half so the server sees EOF and finishes */
    pthread_join(tid, NULL);  /* wait for the reader thread so the server's final output is fully displayed before we quit */
    close(sock);              /* release the socket completely */
    return 0;                 /* success exit code */
}
