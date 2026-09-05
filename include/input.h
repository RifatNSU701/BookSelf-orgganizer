#ifndef INPUT_H
#define INPUT_H

/*
 * input.h
 * Robust console input helpers. All user input flows through here so that
 * invalid menu choices, empty strings, overly long lines and EOF are handled
 * consistently in one place.
 */

#include <stddef.h>

/*
 * input_menu_choice
 * Prompts and reads an integer menu choice in [min,max]. Re-prompts on invalid
 * input. Returns the choice, or -1 on EOF (so the caller can exit cleanly).
 */
int input_menu_choice(const char *prompt, int min, int max);

/*
 * input_string
 * Prompts and reads a non-empty line into 'buf' (size 'size'). Returns 1 on
 * success, 0 on EOF. Rejects empty input by re-prompting.
 */
int input_string(const char *prompt, char *buf, size_t size);

/*
 * input_int
 * Prompts and reads an integer in [min,max]. Re-prompts on invalid input.
 * Returns 1 on success (value in *out), 0 on EOF.
 */
int input_int(const char *prompt, int min, int max, int *out);

#endif /* INPUT_H */
