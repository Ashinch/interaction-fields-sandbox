#include <stdio.h>
#include <seccomp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "main.h"


int main(int argc, char **argv) {
    int opt, seccomp_is_open = 0;
    char *exe_path, *arg;
    int max_cpu_time, max_real_time = 0;
    long max_memory = 0;

    while (EOF != (opt = getopt(argc, argv, "sb:p:c:r:m:"))) {
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
            case 'c':
                max_cpu_time = atoi(optarg);
                break;
            case 'r':
                max_real_time = atoi(optarg);
                break;
            case 'm':
                max_memory = atol(optarg);
                break;
            default:
                return ARGUMENTS_FAILED;
        }
    }
    char **exe_argv = NULL;
    if (arg != NULL) {
        exe_argv = split(arg, ' ');
    }

    struct result _result = {0, 0, 0, 0, 0, 0};
    struct timeval start, end;
    gettimeofday(&start, NULL);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        // Fork failed
        return FORK_FAILED;
    } else if (child_pid == 0) {
        // Child process
        scmp_filter_ctx ctx = NULL;
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
            if (!ctx) return INIT_SECCOMP_FAILED;

            // Add rules for black list
            for (int i = 0, length = sizeof(black_list) / sizeof(int); i < length; i++) {
                if (seccomp_rule_add(ctx, SCMP_ACT_KILL, black_list[i], 0) != 0) {
                    return ADD_BLACK_LIST_FAILED;
                }
            }

            // add extra rule for execve
            if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 1,
                                 SCMP_A0(SCMP_CMP_NE, (scmp_datum_t)(exe_path))) != 0) {
                return ADD_BLACK_LIST_FAILED;
            }

            // do not allow "w" and "rw"
            if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(open), 1,
                                 SCMP_CMP(1, SCMP_CMP_MASKED_EQ, O_WRONLY, O_WRONLY)) != 0) {
                return ADD_BLACK_LIST_FAILED;
            }
            if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(open), 1,
                                 SCMP_CMP(1, SCMP_CMP_MASKED_EQ, O_RDWR, O_RDWR)) !=
                0) {
                return ADD_BLACK_LIST_FAILED;
            }

            // Loads the filter into the kernel
            if (seccomp_load(ctx) != 0) {
                return LOAD_BLACK_LIST_FAILED;
            }
        }

        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        // Run the exe
        execve(exe_path, exe_argv, NULL);

        // Free
        if (ctx != NULL) {
            seccomp_release(ctx);
        }

        if (exe_argv != NULL) {
            for (int i = 0; *(arg + i); i++) {
                free(*(exe_argv + i));
            }
            free(exe_argv);
        }
    } else if (child_pid > 0) {
        // Oneself process
        struct timeout_killer_args killer_args;
        killer_args.timeout = max_real_time;
        killer_args.pid = child_pid;
        pthread_t tid = 0;
        if (pthread_create(&tid, NULL, timeout_killer, (void *) (&killer_args)) != 0) {
            kill(child_pid, SIGKILL);
            return -1;
        }

        int status;
        struct rusage resource_usage;

        if (wait4(child_pid, &status, WSTOPPED, &resource_usage) == -1) {
            kill(child_pid, SIGKILL);
            return -1;
        }

        // get end time
        gettimeofday(&end, NULL);
        _result.real_time = (int) (end.tv_sec * 1000 + end.tv_usec / 1000 - start.tv_sec * 1000 - start.tv_usec / 1000);

        close(pipefd[1]);
        size_t size;
        if (read_all(pipefd[0], &_result.result, &size) < 0) {
            perror("Error reading");
            _result.result = RUNTIME_ERROR.text;
            _result.status_code = RUNTIME_ERROR.code;
        } else {
            fflush(stdout);
            fflush(stderr);
        }

        // process exited, we may need to cancel timeout killer thread
        if (max_real_time != -1) {
            if (pthread_cancel(tid) != 0) {
                // todo logging
            }
        }

        if (WIFSIGNALED(status) != 0) {
            _result.signal = WTERMSIG(status);
        }

        if (_result.signal == SIGUSR1) {
            _result.result = SYSTEM_ERROR.text;
            _result.status_code = SYSTEM_ERROR.code;
        } else {
            _result.cpu_time = (int) (resource_usage.ru_utime.tv_sec * 1000 +
                                      resource_usage.ru_utime.tv_usec / 1000);
            _result.memory = resource_usage.ru_maxrss * 1024;

            _result.status_code = WEXITSTATUS(status) + 1;

            if (_result.signal == SIGSEGV) {
                if (max_memory != -1 && _result.memory > max_memory) {
                    _result.result = MEMORY_LIMIT_EXCEEDED.text;
                    _result.status_code = MEMORY_LIMIT_EXCEEDED.code;
                } else {
                    _result.result = RUNTIME_ERROR.text;
                    _result.status_code = RUNTIME_ERROR.code;
                }
            } else {
                if (_result.signal != 0) {
                    _result.result = RUNTIME_ERROR.text;
                    _result.status_code = RUNTIME_ERROR.code;
                }
                if (max_memory != -1 && _result.memory > max_memory) {
                    _result.result = MEMORY_LIMIT_EXCEEDED.text;
                    _result.status_code = MEMORY_LIMIT_EXCEEDED.code;
                }
                if (max_real_time != -1 && _result.real_time > max_real_time) {
                    _result.result = REAL_TIME_LIMIT_EXCEEDED.text;
                    _result.status_code = REAL_TIME_LIMIT_EXCEEDED.code;
                }
                if (max_cpu_time != -1 && _result.cpu_time > max_cpu_time) {
                    _result.result = CPU_TIME_LIMIT_EXCEEDED.text;
                    _result.status_code = CPU_TIME_LIMIT_EXCEEDED.code;
                }
            }
        }
    }

    _result.result = escape_json(_result.result);

    printf("{\n"
           "    \"cpuTime\": %d,\n"
           "    \"realTime\": %d,\n"
           "    \"memory\": %ld,\n"
           "    \"signal\": %d,\n"
           "    \"statusCode\": %d,\n"
           "    \"result\": \"%s\"\n"
           "}",
           _result.cpu_time,
           _result.real_time,
           _result.memory,
           _result.signal,
           _result.status_code,
           _result.result);

    free(_result.result);

    return _result.status_code == 1 ? 0 : -1;
}
