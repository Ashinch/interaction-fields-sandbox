#ifndef RUN_KILLER_H
#define RUN_KILLER_H

struct timeout_killer_args {
    int pid;
    int timeout;
};

int kill_pid(pid_t pid);

void *timeout_killer(void *timeout_killer_args);

#endif //RUN_KILLER_H
