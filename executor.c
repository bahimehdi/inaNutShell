#include "executor.h"

#include "builtins.h"
#include "custom.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void close_pipes(int pipes[][2], int count)
{
    int i;

    for (i = 0; i < count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

static void unknown_command(const char *name)
{
    fprintf(stderr, "Commande inconnue : %s\n", name);
}

static void apply_redirections(Command *cmd)
{
    int fd;
    int flags;

    if (cmd->infile != NULL) {
        fd = open(cmd->infile, O_RDONLY);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            exit(1);
        }
        close(fd);
    }
    if (cmd->outfile != NULL) {
        flags = O_WRONLY | O_CREAT;
        flags |= cmd->append ? O_APPEND : O_TRUNC;
        fd = open(cmd->outfile, flags, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            exit(1);
        }
        close(fd);
    }
}

static void execute_child(Command *cmd)
{
    /* Restaurer l'action par défaut des signaux : ainsi Ctrl+C (SIGINT) et
       Ctrl+\ (SIGQUIT) interrompent normalement la commande lancée, alors
       que le shell parent, lui, les a interceptés ou ignorés. */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    apply_redirections(cmd);
    if (is_custom(cmd->argv[0]))
        exit(exec_custom(cmd));
    execvp(cmd->argv[0], cmd->argv);
    unknown_command(cmd->argv[0]);
    exit(1);
}

static void exec_single(Command *cmd)
{
    pid_t pid;
    int status;

    if (cmd->argc == 0)
        return;
    if (is_builtin(cmd->argv[0])) {
        exec_builtin(cmd);
        return;
    }
    if (is_custom(cmd->argv[0])) {
        exec_custom(cmd);
        return;
    }
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0)
        execute_child(cmd);
    if (cmd->background) {
        printf("[bg] PID %d started\n", (int)pid);
        fflush(stdout);
    } else {
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
            ;
    }
}

static int create_pipes(int pipes[][2], int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            close_pipes(pipes, i);
            return -1;
        }
    }
    return 0;
}

static void setup_pipeline_child(Pipeline *pipeline, int pipes[][2], int index)
{
    int pipe_count;

    pipe_count = pipeline->n - 1;
    if (index > 0 && dup2(pipes[index - 1][0], STDIN_FILENO) < 0) {
        perror("dup2");
        close_pipes(pipes, pipe_count);
        exit(1);
    }
    if (index < pipeline->n - 1 && dup2(pipes[index][1], STDOUT_FILENO) < 0) {
        perror("dup2");
        close_pipes(pipes, pipe_count);
        exit(1);
    }
    close_pipes(pipes, pipe_count);
    execute_child(&pipeline->cmds[index]);
}

static void wait_for_children(pid_t pids[], int count)
{
    int i;
    int status;
    pid_t waited;

    for (i = 0; i < count; i++) {
        do {
            waited = waitpid(pids[i], &status, 0);
        } while (waited < 0 && errno == EINTR);
    }
}

static void exec_multi(Pipeline *pipeline)
{
    int pipes[MAX_PIPES - 1][2];
    pid_t pids[MAX_PIPES];
    int i;
    int pipe_count;

    pipe_count = pipeline->n - 1;
    if (create_pipes(pipes, pipe_count) < 0)
        return;
    for (i = 0; i < pipeline->n; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            close_pipes(pipes, pipe_count);
            wait_for_children(pids, i);
            return;
        }
        if (pids[i] == 0)
            setup_pipeline_child(pipeline, pipes, i);
    }
    close_pipes(pipes, pipe_count);
    if (pipeline->background) {
        printf("[bg] PID %d started\n", (int)pids[0]);
        fflush(stdout);
    } else {
        wait_for_children(pids, pipeline->n);
    }
}

void exec_pipeline(Pipeline *pipeline)
{
    if (pipeline == NULL || pipeline->n == 0)
        return;
    if (pipeline->n == 1)
        exec_single(&pipeline->cmds[0]);
    else
        exec_multi(pipeline);
}
