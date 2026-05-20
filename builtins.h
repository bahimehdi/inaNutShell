#ifndef BUILTINS_H
#define BUILTINS_H

#include "parser.h"

int is_builtin(char *cmd);
int exec_builtin(Command *cmd);
void history_add(const char *line);
int history_get(int index, const char **out_line);
void history_print(void);

#endif
