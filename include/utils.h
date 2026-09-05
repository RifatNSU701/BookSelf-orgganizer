#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define MAX_TITLE_LEN 128
#define MAX_AUTHOR_LEN 96
#define MAX_GENRE_LEN 48
#define MAX_LINE_LEN 512

void safe_strcpy(char *dst, const char *src, size_t dst_size);
char *trim(char *s);
int str_casecmp_portable(const char *a, const char *b);
int str_casestr_portable(const char *haystack, const char *needle);
int read_line(char *buf, size_t size, void *stream);

#endif
