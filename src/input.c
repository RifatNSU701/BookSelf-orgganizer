#include "input.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int input_string(const char *prompt, char *buf, size_t size)
{
    if (buf == NULL || size == 0) {
        return 0;
    }
    for (;;) {
        if (prompt != NULL) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if (!read_line(buf, size, stdin)) {
            return 0;
        }
        {
            char *t = trim(buf);
            if (t[0] != '\0') {
                memmove(buf, t, strlen(t) + 1);
                return 1;
            }
        }
        printf("Input cannot be empty. Please try again.\n");
    }
}

int input_int(const char *prompt, int min, int max, int *out)
{
    char line[64];
    if (out == NULL) {
        return 0;
    }
    for (;;) {
        char *end;
        long v;
        if (prompt != NULL) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if (!read_line(line, sizeof(line), stdin)) {
            return 0;
        }
        {
            char *t = trim(line);
            if (t[0] == '\0') {
                printf("Please enter a number.\n");
                continue;
            }
            v = strtol(t, &end, 10);
            if (*end != '\0') {
                printf("Invalid number. Please try again.\n");
                continue;
            }
            if (v < (long)min || v > (long)max) {
                printf("Please enter a value between %d and %d.\n", min, max);
                continue;
            }
            *out = (int)v;
            return 1;
        }
    }
}

int input_menu_choice(const char *prompt, int min, int max)
{
    int choice;
    if (!input_int(prompt, min, max, &choice)) {
        return -1;
    }
    return choice;
}
