#include "signals.h"

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stddef.h>

/* SIGCHLD : un processus fils s'est terminé. On le récupère avec waitpid
   pour éviter les processus zombies. WNOHANG : ne pas bloquer s'il n'y a
   rien à récupérer. */
void sigchld_handler(int sig)
{
    int saved_errno;

    (void)sig;
    saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

/* SIGINT (Ctrl+C) : le shell ne doit PAS se terminer. On affiche seulement
   un saut de ligne ; la boucle principale de main.c réaffichera l'invite.
   Le processus fils au premier plan, lui, reçoit SIGINT avec son action
   par défaut (voir executor.c) et est donc interrompu. */
void sigint_handler(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);
}

void setup_signals(void)
{
    struct sigaction sa_chld;
    struct sigaction sa_int;

    /* SIGCHLD : SA_RESTART relance les appels système interrompus ;
       SA_NOCLDSTOP : ne pas notifier quand un fils est seulement stoppé. */
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);

    /* SIGINT : volontairement SANS SA_RESTART, pour que la lecture de
       l'entrée (fgets) soit interrompue et que l'invite soit réaffichée. */
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    /* SIGQUIT (Ctrl+\) : ignoré par le shell. */
    signal(SIGQUIT, SIG_IGN);
}
