#define _GNU_SOURCE
#define _POSIX_SOURCE

#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "runner.h"
#include "main.h"
#include "child.h"
#include "killer.h"
#include "utils.h"

struct status_code CPU_TIME_LIMIT_EXCEEDED = {"CPU_TIME_LIMIT_EXCEEDED", 3};
struct status_code REAL_TIME_LIMIT_EXCEEDED = {"REAL_TIME_LIMIT_EXCEEDED", 3};
struct status_code MEMORY_LIMIT_EXCEEDED = {"MEMORY_LIMIT_EXCEEDED", 4};
struct status_code RUNTIME_ERROR = {"RUNTIME_ERROR", 2};
struct status_code SYSTEM_ERROR = {"SYSTEM_ERROR", 2};

void run(struct config *_config, struct result *_result) {
    // record current time
    struct timeval start, end;
    gettimeofday(&start, NULL);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        exit(FORK_ERROR);
    } else if (child_pid == 0) {
        // Merge standard output/error file descriptor
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        child_process(_config, pipefd);
    } else if (child_pid > 0) {
        // create new thread to monitor process running time
        struct timeout_killer_args killer_args;
        killer_args.timeout = _config->max_real_time;
        killer_args.pid = child_pid;

        pthread_t tid = 0;
        if (pthread_create(&tid, NULL, timeout_killer, (void *) (&killer_args)) != 0) {
            kill_pid(child_pid);
            perror("pthread_create kill_pid");
            exit(EXIT_FAILURE);
        }
        int status;
        struct rusage resource_usage;

        // wait for child process to terminate
        // on success, returns the process ID of the child whose state has changed;
        // On error, -1 is returned.
        if (wait4(child_pid, &status, WSTOPPED, &resource_usage) == -1) {
            kill_pid(child_pid);
            perror("wait4 kill_pid");
            exit(EXIT_FAILURE);
        }

        // get end time
        gettimeofday(&end, NULL);
        _result->real_time = (int) (end.tv_sec * 1000 + end.tv_usec / 1000 - start.tv_sec * 1000 -
                                    start.tv_usec / 1000);

        close(pipefd[1]);
        size_t size;
        if (read_all(pipefd[0], &_result->result, &size) < 0) {
            exit(READ_FD_ERROR);
            _result->result = RUNTIME_ERROR.text;
            _result->status_code = RUNTIME_ERROR.code;
        } else {
            fflush(stdout);
            fflush(stderr);
        }

        // process exited, we may need to cancel timeout killer thread
        if (_config->max_real_time != -1) {
            if (pthread_cancel(tid) != 0) {
                exit(TIMEOUT_KILLER_THREAD_CANCEL_ERROR);
            }
        }

        if (WIFSIGNALED(status) != 0) {
            _result->signal = WTERMSIG(status);
        }

        if (_result->signal == SIGUSR1) {
            _result->result = SYSTEM_ERROR.text;
            _result->status_code = SYSTEM_ERROR.code;
        } else {
            _result->cpu_time = (int) (resource_usage.ru_utime.tv_sec * 1000 +
                                       resource_usage.ru_utime.tv_usec / 1000);
            // Unit: KB
            _result->memory = resource_usage.ru_maxrss;

            // To correspond to a primary key field in a database table (the ID primary key starts at 1)
            _result->status_code = WEXITSTATUS(status) + 1;

//            if (_result->signal == SIGSEGV) {
//                if (_config->max_memory != -1 && _result->memory > _config->max_memory) {
//                    _result->result = MEMORY_LIMIT_EXCEEDED.text;
//                    _result->status_code = MEMORY_LIMIT_EXCEEDED.code;
//                } else {
//                    _result->result = RUNTIME_ERROR.text;
//                    _result->status_code = RUNTIME_ERROR.code;
//                }
//            } else {
                if (_result->signal != 0) {
                    _result->result = RUNTIME_ERROR.text;
                    _result->status_code = RUNTIME_ERROR.code;
                }
                if (_config->max_memory != -1 && _result->memory > _config->max_memory) {
                    _result->result = MEMORY_LIMIT_EXCEEDED.text;
                    _result->status_code = MEMORY_LIMIT_EXCEEDED.code;
                }
                if (_config->max_real_time != -1 && _result->real_time > _config->max_real_time) {
                    _result->result = REAL_TIME_LIMIT_EXCEEDED.text;
                    _result->status_code = REAL_TIME_LIMIT_EXCEEDED.code;
                }
                if (_config->max_cpu_time != -1 && _result->cpu_time > _config->max_cpu_time) {
                    _result->result = CPU_TIME_LIMIT_EXCEEDED.text;
                    _result->status_code = CPU_TIME_LIMIT_EXCEEDED.code;
                }
//            }
        }
    }
}
