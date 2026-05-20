#ifndef SIGNALS_H
#define SIGNALS_H

void setup_signals(void);
void sigchld_handler(int sig);
void sigint_handler(int sig);

#endif
