#ifndef UTILS_H
#define UTILS_H

void print_prompt(void);
char *read_line(void);
char *xstrdup(const char *s);
int is_blank_line(const char *line);
char *trim_whitespace(char *s);

#endif
