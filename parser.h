#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 128
#define MAX_PIPES 32

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    char *outfile;
    int append;
    char *infile;
    int background;
} Command;

typedef struct {
    Command cmds[MAX_PIPES];
    int n;
    int background;
} Pipeline;

void init_command(Command *cmd);
void init_pipeline(Pipeline *pipeline);
int parse_line(const char *line, Pipeline *pipeline);
void free_command(Command *cmd);
void free_pipeline(Pipeline *pipeline);

#endif
