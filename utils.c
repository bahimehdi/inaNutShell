#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 4096

void print_prompt(void)
{
    printf("inaNutShell> ");
    fflush(stdout);
}

char *read_line(void)
{
    char buffer[LINE_SIZE];
    size_t len;

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        if (feof(stdin))
            return NULL;        /* vraie fin de fichier (Ctrl+D) */
        clearerr(stdin);        /* interruption par un signal (Ctrl+C) */
        return xstrdup("");     /* ligne vide : la boucle réaffiche l'invite */
    }
    len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[len - 1] = '\0';
        len--;
    }
    if (len == sizeof(buffer) - 1) {
        int c;

        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
    return xstrdup(buffer);
}

char *xstrdup(const char *s)
{
    char *copy;
    size_t len;

    if (s == NULL)
        return NULL;
    len = strlen(s) + 1;
    copy = malloc(len);
    if (copy == NULL) {
        perror("malloc");
        exit(1);
    }
    memcpy(copy, s, len);
    return copy;
}

int is_blank_line(const char *line)
{
    if (line == NULL)
        return 1;
    while (*line != '\0' && isspace((unsigned char)*line))
        line++;
    return *line == '\0';
}

char *trim_whitespace(char *s)
{
    char *end;

    if (s == NULL)
        return NULL;
    while (*s != '\0' && isspace((unsigned char)*s))
        s++;
    if (*s == '\0')
        return s;
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    return s;
}
