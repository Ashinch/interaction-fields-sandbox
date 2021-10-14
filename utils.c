#define _POSIX_SOURCE

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "utils.h"

char **split(char *a_str, char a_delim) {
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

char *escape_json(char *str) {
    if (str == NULL) {
        return "";
    }
    char tmp[200];
    strncpy(tmp, str, 200);
    tmp[199] = '\0';
    return replaceAll(replaceAll(replaceAll(replaceAll(tmp, "\\", "\\\\"), "\"", "\\\""), "\n", "\\n"), "\t", "\\t");
}
