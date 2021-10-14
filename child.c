#define _DEFAULT_SOURCE
#define _POSIX_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "child.h"
#include "rules/seccomp_rules.h"
#include "runner.h"

void child_process(struct config *_config, int pipefd[]) {
    // Load seccomp
    if (_config->seccomp_rule_name != NULL) {
        if (strcmp("general", _config->seccomp_rule_name) == 0) {
            general_seccomp_rules(_config->exe_path);
        }
    }

    // Run it
    execve(_config->exe_path, _config->exe_argv, NULL);

    // Free argv
    if (_config->exe_argv != NULL) {
        for (int i = 0; *(_config->exe_argv + i); i++) {
            free(*(_config->exe_argv + i));
        }
        free(_config->exe_argv);
    }
}
