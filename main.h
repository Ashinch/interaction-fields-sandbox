#ifndef RUN_MAIN_H
#define RUN_MAIN_H

enum {
    ARGUMENTS_ERROR = -1,
    FORK_ERROR = -2,
    LOAD_SECCOMP_ERROR = -3,
    READ_FD_ERROR = -4,
    TIMEOUT_KILLER_THREAD_CANCEL_ERROR = -5,
};

//char *strrepl(const char *src, char *dst, size_t dst_size, const char *search, const char *replace_with) {
//    char *replace_buf = (char *) malloc(dst_size);
//    if (replace_buf) {
//        replace_buf[0] = 0;
//        char *p = (char *) src;
//        char *pos = NULL;
//        while ((pos = strstr(p, search)) != NULL) {
//            size_t n = (size_t) (pos - p);
//            strncat(replace_buf, p, n > dst_size ? dst_size : n);
//            strncat(replace_buf, replace_with, dst_size - strlen(replace_buf) - 1);
//            p = pos + strlen(search);
//        }
//        snprintf(dst, dst_size, "%s%s", replace_buf, p);
//        free(replace_buf);
//    }
//    return dst;
//}


#endif //RUN_MAIN_H
