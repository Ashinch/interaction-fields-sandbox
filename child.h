#ifndef RUN_CHILD_H
#define RUN_CHILD_H

#include "runner.h"

void child_process(struct config *_config, int pipefd[]);

#endif //RUN_CHILD_H
