#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

/*
 * process_manager.h
 * Demonstrates genuine OS process management with fork()/wait() on POSIX
 * environments (Cygwin, MSYS2, Linux, macOS).
 *
 * Architecture:
 *   Parent process  -> coordinates and waits.
 *     Child 1 (Organization child): reloads the inventory from disk into its
 *              OWN address space and runs a sort, proving processes have
 *              separate memory (the parent's copy is untouched).
 *     Child 2 (Reporting child): produces a short report from the same file.
 *   Parent uses waitpid() to collect both children and reads their exit codes.
 *
 * PORTABILITY:
 *   fork() is a POSIX system call. Native MinGW does NOT provide it. On a
 *   native-MinGW build we compile a HONEST fallback (guarded by the
 *   HAVE_FORK macro) that clearly reports the limitation instead of faking a
 *   process. See README.md and docs/architecture.md.
 */

#include "inventory.h"

/*
 * run_process_demo
 * Runs the parent/child process demonstration described above, operating on
 * the on-disk data file 'data_path'. Returns 0 on success, -1 on error.
 *
 * When compiled without fork() support it prints an explicit, academically
 * honest notice and returns 1 (meaning "demo skipped due to platform").
 */
int run_process_demo(const char *data_path);

#endif /* PROCESS_MANAGER_H */
