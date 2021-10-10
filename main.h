//
// Created by Ashinch on 2021/9/26.
//

#ifndef INTERACTION_FIELDS_SANDBOX_MAIN_H
#define INTERACTION_FIELDS_SANDBOX_MAIN_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <regex.h>

enum {
    ARGUMENTS_FAILED = -1,
    FORK_FAILED = -2,
    INIT_SECCOMP_FAILED = -3,
    ADD_BLACK_LIST_FAILED = -4,
    LOAD_BLACK_LIST_FAILED = -5,
};

struct timeout_killer_args {
    int pid;
    int timeout;
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
    char *text;
    int code;
};

struct status_code CPU_TIME_LIMIT_EXCEEDED = {"CPU_TIME_LIMIT_EXCEEDED", 3};
struct status_code REAL_TIME_LIMIT_EXCEEDED = {"REAL_TIME_LIMIT_EXCEEDED", 3};
struct status_code MEMORY_LIMIT_EXCEEDED = {"MEMORY_LIMIT_EXCEEDED", 4};
struct status_code RUNTIME_ERROR = {"RUNTIME_ERROR", 2};
struct status_code SYSTEM_ERROR = {"SYSTEM_ERROR", 2};

char **split(char *a_str, const char a_delim) {
    char **result = 0;
    size_t count = 0;
    char *tmp = a_str;
    char *last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char *) * count);

    if (result) {
        size_t idx = 0;
        char *token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void *timeout_killer(void *timeout_killer_args) {
    // this is a new thread, kill the process if timeout
    pid_t pid = ((struct timeout_killer_args *) timeout_killer_args)->pid;
    int timeout = ((struct timeout_killer_args *) timeout_killer_args)->timeout;
    // On success, pthread_detach() returns 0; on error, it returns an error number.
    if (pthread_detach(pthread_self()) != 0) {
        kill(pid, SIGKILL);
        return NULL;
    }
    // usleep can't be used, for time args must < 1000ms
    // this may sleep longer that expected, but we will have a check at the end
    if (sleep((unsigned int) ((timeout + 1000) / 1000)) != 0) {
        kill(pid, SIGKILL);
        return NULL;
    }
    if (kill(pid, SIGKILL) != 0) {
        return NULL;
    }
    return NULL;
}

int read_all(int fd, char **buf_ptr, size_t *size_ptr) {
    const size_t BLOCK_SIZE = 64 * 1024;

    *size_ptr = 0;
    *buf_ptr = NULL;

    while (1) {
        void *new_buf = realloc(*buf_ptr, *size_ptr + BLOCK_SIZE);
        if (!new_buf) {
            *buf_ptr = realloc(*buf_ptr, *size_ptr);  // Free unused space.
            return 0;
        }

        *buf_ptr = new_buf;
        char *p = ((char *) *buf_ptr) + *size_ptr;
        ssize_t bytes_read = read(fd, p, BLOCK_SIZE);
        if (bytes_read <= 0) {
            *buf_ptr = realloc(*buf_ptr, *size_ptr);  // Free unused space.
            return bytes_read;
        }

        *size_ptr += bytes_read;
    }
}

int getFindStrCount(char *src, char *find) {
    int count = 0;
    char *position = src;
    int findLen = strlen(find);
    while ((position = strstr(position, find)) != NULL) {
        count++;
        position = position + findLen;
    }
    return count;
}

char *replaceAll(char *src, char *find, char *replaceWith) {
    //如果find或者replace为null，则返回和src一样的字符串。
    if (find == NULL || replaceWith == NULL) {
        return strdup(src);
    }
    //指向替换后的字符串的head。
    char *afterReplaceHead = NULL;
    //总是指向新字符串的结尾位置。
    char *afterReplaceIndex = NULL;
    //find字符串在src字符串中出现的次数
    int count = 0;
    int i, j, k;

    int srcLen = strlen(src);
    int findLen = strlen(find);
    int replaceWithLen = strlen(replaceWith);

    //指向src字符串的某个位置，从该位置开始复制子字符串到afterReplaceIndex，初始从src的head开始复制。
    char *srcIndex = src;
    //src字符串的某个下标，从该下标开始复制字符串到afterReplaceIndex，初始为src的第一个字符。
    int cpStrStart = 0;

    //获取find字符串在src字符串中出现的次数
    count = getFindStrCount(src, find);
    //如果没有出现，则返回和src一样的字符串。
    if (count == 0) {
        return strdup(src);
    }

    //为新字符串申请内存
    afterReplaceHead = afterReplaceIndex = (char *) malloc(srcLen + 1 + (replaceWithLen - findLen) * count);
    //初始化新字符串内存
    memset(afterReplaceHead, '\0', sizeof(afterReplaceHead));

    for (i = 0, j = 0, k = 0; i != srcLen; i++) {
        //如果find字符串的字符和src中字符串的字符是否相同。
        if (src[i] == find[j]) {
            //如果刚开始比较，则将i的值先赋给k保存。
            if (j == 0) {
                k = i;
            }
            //如果find字符串包含在src字符串中
            if (j == (findLen - 1)) {
                j = 0;
                //拷贝src中find字符串之前的字符串到新字符串中
                strncpy(afterReplaceIndex, srcIndex, i - findLen - cpStrStart + 1);
                //修改afterReplaceIndex
                afterReplaceIndex = afterReplaceIndex + i - findLen - cpStrStart + 1;
                //修改srcIndex
                srcIndex = srcIndex + i - findLen - cpStrStart + 1;
                //cpStrStart
                cpStrStart = i + 1;

                //拷贝replaceWith字符串到新字符串中
                strncpy(afterReplaceIndex, replaceWith, replaceWithLen);
                //修改afterReplaceIndex
                afterReplaceIndex = afterReplaceIndex + replaceWithLen;
                //修改srcIndex
                srcIndex = srcIndex + findLen;
            } else {
                j++;
            }
        } else {
            //如果find和src比较过程中出现不相等的情况，则将保存的k值还给i
            if (j != 0) {
                i = k;
            }
            j = 0;
        }
    }
    //最后将src中最后一个与find匹配的字符串后面的字符串复制到新字符串中。
    strncpy(afterReplaceIndex, srcIndex, i - cpStrStart);

    return afterReplaceHead;
}


char *strrepl(const char *src, char *dst, size_t dst_size, const char *search, const char *replace_with) {
    char *replace_buf = (char *) malloc(dst_size);
    if (replace_buf) {
        replace_buf[0] = 0;
        char *p = (char *) src;
        char *pos = NULL;
        while ((pos = strstr(p, search)) != NULL) {
            size_t n = (size_t) (pos - p);
            strncat(replace_buf, p, n > dst_size ? dst_size : n);
            strncat(replace_buf, replace_with, dst_size - strlen(replace_buf) - 1);
            p = pos + strlen(search);
        }
        snprintf(dst, dst_size, "%s%s", replace_buf, p);
        free(replace_buf);
    }
    return dst;
}

char *escape_json(char *str) {
    char tmp[200];
    strncpy(tmp, str, 200);
    tmp[199] = '\0';
    return replaceAll(replaceAll(replaceAll(tmp, "\\", "\\\\"), "\"", "\\\""), "\n", "\\n");
}


#endif //INTERACTION_FIELDS_SANDBOX_MAIN_H
