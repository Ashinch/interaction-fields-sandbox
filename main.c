#include <stdio.h>
#include <seccomp.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"


int main(int argc, char **argv) {
    int opt, seccomp_is_open = 0;
    char *exe_path, *arg;

    while (EOF != (opt = getopt(argc, argv, "sb:p:"))) {
        switch (opt) {
            case 's':
                seccomp_is_open = 1;
                break;
            case 'b':
                exe_path = optarg;
                break;
            case 'p':
                arg = optarg;
                break;
            default:
                return ARGUMENTS_ERROR;
        }
    }
    char **exe_argv = NULL;
    if (arg != NULL) {
        exe_argv = split(arg, ' ');
    }

    scmp_filter_ctx ctx = NULL;
    // Seccomp open
    if (seccomp_is_open == 1) {
        // System Call Blacklist
        int black_list[] = {
                SCMP_SYS(clone),
                SCMP_SYS(fork),
                SCMP_SYS(vfork),
                SCMP_SYS(kill),
                SCMP_SYS(socket)
        };

        // Initialize seccomp
        ctx = seccomp_init(SCMP_ACT_ALLOW);
        if (!ctx) return INIT_SECCOMP_ERROR;

        // Add rules for black list
        for (int i = 0, length = sizeof(black_list) / sizeof(int); i < length; i++) {
            if (seccomp_rule_add(ctx, SCMP_ACT_KILL, black_list[i], 0) != 0) {
                return ADD_BLACK_LIST_ERROR;
            }
        }

        // add extra rule for execve
        if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 1, SCMP_A0(SCMP_CMP_NE, (scmp_datum_t)(exe_path)))
        !=
        0) {
            return ADD_BLACK_LIST_ERROR;
        }

        // do not allow "w" and "rw"
        if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(open), 1,
                             SCMP_CMP(1, SCMP_CMP_MASKED_EQ, O_WRONLY, O_WRONLY)) != 0) {
            return ADD_BLACK_LIST_ERROR;
        }
        if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(open), 1, SCMP_CMP(1, SCMP_CMP_MASKED_EQ, O_RDWR, O_RDWR)) !=
            0) {
            return ADD_BLACK_LIST_ERROR;
        }

        // Loads the filter into the kernel
        if (seccomp_load(ctx) != 0) {
            return LOAD_BLACK_LIST_ERROR;
        }
    }

    // Run the exe
    execve(exe_path, exe_argv, NULL);

    // Free
    if (exe_argv != NULL) {
        for (int i = 0; *(arg + i); i++) {
            free(*(exe_argv + i));
        }
        free(exe_argv);
    }
    if (ctx != NULL) {
        seccomp_release(ctx);
    }
    return 0;
}
