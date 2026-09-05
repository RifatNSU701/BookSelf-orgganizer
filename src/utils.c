#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void safe_strcpy(char *dst, const char *src, size_t dst_size)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

char *trim(char *s)
{
    char *end;

    if (s == NULL) {
        return s;
    }
    /* Skip leading whitespace. */
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    if (*s == '\0') {
        return s; /* all spaces */
    }
    /* Trim trailing whitespace. */
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

int str_casecmp_portable(const char *a, const char *b)
{
    unsigned char ca, cb;

    if (a == NULL || b == NULL) {
        return (a == b) ? 0 : (a == NULL ? -1 : 1);
    }
    while (*a != '\0' && *b != '\0') {
        ca = (unsigned char)tolower((unsigned char)*a);
        cb = (unsigned char)tolower((unsigned char)*b);
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        a++;
        b++;
    }
    return (int)(unsigned char)tolower((unsigned char)*a)
         - (int)(unsigned char)tolower((unsigned char)*b);
}

int str_casestr_portable(const char *haystack, const char *needle)
{
    size_t nlen, i;

    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    if (needle[0] == '\0') {
        return 1; /* empty needle matches everything */
    }
    nlen = strlen(needle);

    for (i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while (j < nlen && haystack[i + j] != '\0') {
            unsigned char hc = (unsigned char)tolower((unsigned char)haystack[i + j]);
            unsigned char nc = (unsigned char)tolower((unsigned char)needle[j]);
            if (hc != nc) {
                break;
            }
            j++;
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

int read_line(char *buf, size_t size, void *stream)
{
    FILE *fp = (FILE *)stream;
    size_t len;

    if (buf == NULL || size == 0 || fp == NULL) {
        return 0;
    }
    if (fgets(buf, (int)size, fp) == NULL) {
        return 0; /* EOF or error */
    }
    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0'; /* strip newline */
    } else if (len == size - 1) {
        /* Line longer than the buffer: discard the remainder so the next
         * read starts on a fresh line. */
        int ch;
        while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
            /* discard */
        }
    }
    return 1;
}
