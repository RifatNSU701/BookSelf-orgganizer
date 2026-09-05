#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

int input_menu_choice(const char *prompt, int min, int max);
int input_string(const char *prompt, char *buf, size_t size);
int input_int(const char *prompt, int min, int max, int *out);

#endif
