#include <stdio.h>
#include <seccomp.h>
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv) {
    // System Call Blacklist
    int black_list[] = {
            SCMP_SYS(clone),
            SCMP_SYS(fork),
            SCMP_SYS(vfork),
            SCMP_SYS(kill),
            SCMP_SYS(socket)
    };
    // Initialize seccomp
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!ctx) return INIT_SECCOMP_ERROR;
    // Add rules for black list
    for (int i = 0, length = sizeof(black_list) / sizeof(int); i < length; i++) {
        if (seccomp_rule_add(ctx, SCMP_ACT_KILL, black_list[i], 0) != 0) {
            return ADD_BLACK_LIST_ERROR;
        }
    }
    // Loads the filter into the kernel
    seccomp_load(ctx);
    char *bin = argv[1];
    for (int i = 0, n = sizeof(argv) / sizeof(char); i < n; i++) {
        argv[i] = argv[i + 1];
    }
    // Run the code
    execve(bin, argv, NULL);
    return 0;
}
