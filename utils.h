#ifndef RUN_UTILS_H
#define RUN_UTILS_H

char **split(char *a_str, char a_delim);

int read_all(int fd, char **buf_ptr, size_t *size_ptr);

int getFindStrCount(char *src, char *find);

char *replaceAll(char *src, char *find, char *replaceWith);

char *escape_json(char *str);

#endif //RUN_UTILS_H
