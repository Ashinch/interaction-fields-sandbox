#ifndef RUN_SECCOMP_RULES_H
#define RUN_SECCOMP_RULES_H

#include <stdbool.h>

int _c_cpp_seccomp_rules(char *exe_path, bool allow_write_file);
int c_cpp_seccomp_rules(char *exe_path);
int general_seccomp_rules(char *exe_path);
int c_cpp_file_io_seccomp_rules(char *exe_path);
int golang_seccomp_rules(char *exe_path);
int node_seccomp_rules(char *exe_path);

#endif //RUN_SECCOMP_RULES_H
