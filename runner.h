#ifndef RUN_RUNNER_H
#define RUN_RUNNER_H

struct config {
    char *exe_path;
    char **exe_argv;
    int max_cpu_time;
    int max_real_time;
    long max_memory;
    char *seccomp_rule_name;
};

struct result {
    int cpu_time;
    int real_time;
    long memory;
    int signal;
    int status_code;
    char *result;
};

struct status_code {
    const char *text;
    int code;
};

void run(struct config *_config, struct result *_result);

#endif //RUN_RUNNER_H
