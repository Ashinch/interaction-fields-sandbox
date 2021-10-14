#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "main.h"
#include "runner.h"
#include "utils.h"

int main(int argc, char **argv) {
    struct config _config = {0, 0, 0, 0, 0, 0};
    int opt;
    char *arg;

    while (EOF != (opt = getopt(argc, argv, "s:b:p:c:r:m:"))) {
        switch (opt) {
            case 's':
                _config.seccomp_rule_name = optarg;
                break;
            case 'b':
                _config.exe_path = optarg;
                break;
            case 'p':
                arg = optarg;
                break;
            case 'c':
                _config.max_cpu_time = atoi(optarg);
                break;
            case 'r':
                _config.max_real_time = atoi(optarg);
                break;
            case 'm':
                _config.max_memory = atol(optarg);
                break;
            default:
                exit(ARGUMENTS_ERROR);
        }
    }

    if (arg) {
        _config.exe_argv = split(arg, ' ');
    }

    struct result _result = {0, 0, 0, 0, 0, 0};

    run(&_config, &_result);

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
