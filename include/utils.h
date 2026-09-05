#ifndef UTILS_H
#define UTILS_H

/*
 * utils.h
 * Small helper utilities used across the project:
 *  - safe string copy
 *  - trimming whitespace
 *  - case-insensitive comparison
 *  - reading a line safely from a stream (guards against buffer overflow)
 */

#include <stddef.h>

/* Maximum sizes for various text fields. Centralised to avoid magic numbers. */
#define MAX_TITLE_LEN   128
#define MAX_AUTHOR_LEN   96
#define MAX_GENRE_LEN    48
#define MAX_LINE_LEN    512

/*
 * safe_strcpy
 * Copies at most (dst_size - 1) characters from src into dst and always
 * NUL-terminates dst. Prevents buffer overflow (a classic C pitfall).
 */
void safe_strcpy(char *dst, const char *src, size_t dst_size);

/*
 * trim
 * Removes leading and trailing whitespace from a string in place.
 * Returns the pointer to the first non-space character.
 */
char *trim(char *s);

/*
 * str_casecmp_portable
 * Case-insensitive string comparison. Returns <0, 0, >0 like strcmp.
 * Implemented portably so it works identically on MinGW and POSIX.
 */
int str_casecmp_portable(const char *a, const char *b);

/*
 * str_casestr_portable
 * Case-insensitive substring search. Returns 1 if 'needle' occurs anywhere
 * inside 'haystack' (case-insensitively), 0 otherwise.
 */
int str_casestr_portable(const char *haystack, const char *needle);

/*
 * read_line
 * Reads a single line from 'stream' into 'buf' (size 'size'), stripping the
 * trailing newline. Returns 1 on success, 0 on EOF/error. Discards any
 * remaining characters on the line if the input was longer than the buffer,
 * so subsequent reads are not corrupted.
 */
int read_line(char *buf, size_t size, void *stream);

#endif /* UTILS_H */
