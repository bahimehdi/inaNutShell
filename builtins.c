#include "builtins.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HISTORY_SIZE 100

static char *history[HISTORY_SIZE];
static int history_count;

int is_builtin(char *cmd)
{
    if (cmd == NULL)
        return 0;
    return strcmp(cmd, "cd") == 0 || strcmp(cmd, "exit") == 0
        || strcmp(cmd, "history") == 0;
}

static int builtin_cd(Command *cmd)
{
    char *path;

    if (cmd->argc < 2) {
        path = getenv("HOME");
        if (path == NULL)
            path = "/";
    } else {
        path = cmd->argv[1];
    }
    if (chdir(path) != 0) {
        fprintf(stderr, "cd: %s: No such file or directory\n", path);
        return 1;
    }
    return 0;
}

static int builtin_exit(Command *cmd)
{
    int code;

    code = 0;
    if (cmd->argc >= 2)
        code = atoi(cmd->argv[1]);
    exit(code);
}

void history_add(const char *line)
{
    int slot;

    if (line == NULL || *line == '\0')
        return;
    slot = history_count % HISTORY_SIZE;
    free(history[slot]);
    history[slot] = xstrdup(line);
    history_count++;
}

int history_get(int index, const char **out_line)
{
    int oldest;
    int slot;

    if (out_line == NULL || index <= 0)
        return -1;
    oldest = history_count - HISTORY_SIZE;
    if (oldest < 0)
        oldest = 0;
    if (index <= oldest || index > history_count)
        return -1;
    slot = (index - 1) % HISTORY_SIZE;
    if (history[slot] == NULL)
        return -1;
    *out_line = history[slot];
    return 0;
}

void history_print(void)
{
    int start;
    int i;
    const char *line;

    start = history_count - HISTORY_SIZE + 1;
    if (start < 1)
        start = 1;
    for (i = start; i <= history_count; i++) {
        if (history_get(i, &line) == 0)
            printf("%d  %s\n", i, line);
    }
}

int exec_builtin(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;
    if (strcmp(cmd->argv[0], "cd") == 0)
        return builtin_cd(cmd);
    if (strcmp(cmd->argv[0], "exit") == 0)
        return builtin_exit(cmd);
    if (strcmp(cmd->argv[0], "history") == 0) {
        history_print();
        return 0;
    }
    return 1;
}
