#include "parser.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_command(Command *cmd)
{
    int i;

    cmd->argc = 0;
    cmd->outfile = NULL;
    cmd->append = 0;
    cmd->infile = NULL;
    cmd->background = 0;
    for (i = 0; i < MAX_ARGS; i++)
        cmd->argv[i] = NULL;
}

void init_pipeline(Pipeline *pipeline)
{
    int i;

    pipeline->n = 0;
    pipeline->background = 0;
    for (i = 0; i < MAX_PIPES; i++)
        init_command(&pipeline->cmds[i]);
}

static int add_arg(Command *cmd, const char *token)
{
    const char *value;

    if (cmd->argc >= MAX_ARGS - 1) {
        fprintf(stderr, "parser: too many arguments\n");
        return -1;
    }
    value = token;
    if (token[0] == '$' && token[1] != '\0') {
        value = getenv(token + 1);
        if (value == NULL)
            value = "";
    }
    cmd->argv[cmd->argc] = xstrdup(value);
    cmd->argc++;
    cmd->argv[cmd->argc] = NULL;
    return 0;
}

static int set_file(char **target, const char *token)
{
    free(*target);
    *target = xstrdup(token);
    return 0;
}

static int finish_command(Pipeline *pipeline, Command **current)
{
    if ((*current)->argc == 0) {
        fprintf(stderr, "parser: empty command in pipeline\n");
        return -1;
    }
    pipeline->n++;
    if (pipeline->n >= MAX_PIPES) {
        fprintf(stderr, "parser: too many commands in pipeline\n");
        return -1;
    }
    *current = &pipeline->cmds[pipeline->n];
    return 0;
}

int parse_line(const char *line, Pipeline *pipeline)
{
    char *copy;
    char *token;
    char *next;
    Command *current;

    init_pipeline(pipeline);
    if (line == NULL || is_blank_line(line))
        return 0;
    copy = xstrdup(line);
    current = &pipeline->cmds[0];
    token = strtok(copy, " \t");
    while (token != NULL) {
        if (strcmp(token, "|") == 0) {
            if (finish_command(pipeline, &current) < 0) {
                free(copy);
                free_pipeline(pipeline);
                return -1;
            }
        } else if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0
            || strcmp(token, "<") == 0) {
            next = strtok(NULL, " \t");
            if (next == NULL) {
                fprintf(stderr, "parser: missing file after %s\n", token);
                free(copy);
                free_pipeline(pipeline);
                return -1;
            }
            if (strcmp(token, "<") == 0)
                set_file(&current->infile, next);
            else {
                set_file(&current->outfile, next);
                current->append = (strcmp(token, ">>") == 0);
            }
        } else if (strcmp(token, "&") == 0) {
            next = strtok(NULL, " \t");
            if (next != NULL) {
                fprintf(stderr, "parser: '&' must be the last token\n");
                free(copy);
                free_pipeline(pipeline);
                return -1;
            }
            pipeline->background = 1;
            current->background = 1;
            break;
        } else if (add_arg(current, token) < 0) {
            free(copy);
            free_pipeline(pipeline);
            return -1;
        }
        token = strtok(NULL, " \t");
    }
    if (current->argc > 0) {
        pipeline->n++;
        pipeline->cmds[pipeline->n - 1].background = pipeline->background;
    } else if (pipeline->n > 0) {
        fprintf(stderr, "parser: trailing pipe without command\n");
        free(copy);
        free_pipeline(pipeline);
        return -1;
    }
    free(copy);
    return 0;
}

void free_command(Command *cmd)
{
    int i;

    for (i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }
    free(cmd->outfile);
    free(cmd->infile);
    init_command(cmd);
}

void free_pipeline(Pipeline *pipeline)
{
    int i;

    for (i = 0; i < MAX_PIPES; i++)
        free_command(&pipeline->cmds[i]);
    init_pipeline(pipeline);
}
