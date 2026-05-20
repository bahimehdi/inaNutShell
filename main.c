#include "builtins.h"
#include "executor.h"
#include "parser.h"
#include "signals.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *resolve_history_command(const char *line)
{
    char *copy;
    char *trimmed;
    char *endptr;
    long index;
    const char *resolved;

    copy = xstrdup(line);
    trimmed = trim_whitespace(copy);
    if (trimmed[0] != '!') {
        if (trimmed != copy)
            memmove(copy, trimmed, strlen(trimmed) + 1);
        return copy;
    }
    if (trimmed[1] == '\0') {
        fprintf(stderr, "history: invalid reference\n");
        free(copy);
        return NULL;
    }
    index = strtol(trimmed + 1, &endptr, 10);
    if (*endptr != '\0' || index <= 0) {
        fprintf(stderr, "history: invalid reference\n");
        free(copy);
        return NULL;
    }
    if (history_get((int)index, &resolved) != 0) {
        fprintf(stderr, "history: %ld: not found\n", index);
        free(copy);
        return NULL;
    }
    free(copy);
    return xstrdup(resolved);
}

static void execute_line(const char *line)
{
    char *copy;
    char *segment;
    char *trimmed;
    char *saveptr;
    Pipeline pipeline;

    copy = xstrdup(line);
    /* strtok_r keeps its position in our own 'saveptr', so it does not
       collide with the strtok() used inside parse_line(). */
    segment = strtok_r(copy, ";", &saveptr);
    while (segment != NULL) {
        trimmed = trim_whitespace(segment);
        if (*trimmed != '\0') {
            if (parse_line(trimmed, &pipeline) == 0) {
                exec_pipeline(&pipeline);
                free_pipeline(&pipeline);
            }
        }
        segment = strtok_r(NULL, ";", &saveptr);
    }
    free(copy);
}

int main(void)
{
    char *line;
    char *expanded;

    setup_signals();
    while (1) {
        print_prompt();
        line = read_line();
        if (line == NULL) {
            printf("\n");
            break;
        }
        if (is_blank_line(line)) {
            free(line);
            continue;
        }
        expanded = resolve_history_command(line);
        if (expanded == NULL) {
            free(line);
            continue;
        }
        if (!is_blank_line(expanded))
            history_add(expanded);
        execute_line(expanded);
        free(expanded);
        free(line);
    }
    return 0;
}
