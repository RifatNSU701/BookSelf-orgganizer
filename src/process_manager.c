/*
 * process_manager.c
 * Genuine OS process demonstration using fork()/waitpid() on POSIX systems.
 *
 * HONESTY ABOUT PORTABILITY
 *   fork() is a POSIX system call. It is provided by Linux, macOS, Cygwin and
 *   MSYS2. Native MinGW (plain Windows GCC) does NOT implement fork(). We do
 *   NOT fake a process with a thread. Instead we detect fork() at COMPILE TIME
 *   and, when it is unavailable, print an explicit, academically honest notice.
 *
 *   Detection: __CYGWIN__ or __unix__/__linux__/__APPLE__ imply real fork().
 *   We define HAVE_FORK accordingly. The Code::Blocks native-MinGW target
 *   simply won't define it, and this file compiles to the honest fallback.
 */

#include "process_manager.h"
#include "inventory.h"
#include "sorting.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Decide whether real fork() is available on this build. */
#if defined(__CYGWIN__) || defined(__unix__) || defined(__unix) || \
    defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
#  define HAVE_FORK 1
#else
#  define HAVE_FORK 0
#endif

#if HAVE_FORK
#  include <unistd.h>      /* fork, getpid, getppid, _exit */
#  include <sys/wait.h>    /* waitpid, WIFEXITED, WEXITSTATUS */
#endif

/*
 * child_organization
 * Runs in the child's OWN address space: loads the inventory fresh from disk,
 * sorts it, and reports. Because the child has a separate copy, the parent's
 * in-memory state is completely unaffected — this is the defining difference
 * between processes and threads.
 * Returns an exit code (0 success, non-zero on error).
 */
static int child_organization(const char *data_path)
{
    Inventory local;
    int loaded;

    if (inventory_init(&local) != 0) {
        return 2;
    }
    loaded = inventory_load(&local, data_path);
    if (loaded < 0) {
        inventory_destroy(&local);
        return 3;
    }

    log_event("Child-Organize", "Loaded %d book(s) into my private memory.",
              loaded);

    /* Sort in this process's memory only. */
    pthread_mutex_lock(&local.lock);
    sort_books_by_title(local.books, local.count);
    pthread_mutex_unlock(&local.lock);

    log_event("Child-Organize", "Sorted my private copy by title. "
                                 "Parent's memory is untouched.");
    inventory_destroy(&local);
    return 0;
}

/*
 * child_report
 * Produces a small genre tally from the same file, again in its own memory.
 */
static int child_report(const char *data_path)
{
    Inventory local;
    int loaded, i;
    int cs = 0, fic = 0, other = 0;

    if (inventory_init(&local) != 0) {
        return 2;
    }
    loaded = inventory_load(&local, data_path);
    if (loaded < 0) {
        inventory_destroy(&local);
        return 3;
    }

    for (i = 0; i < local.count; i++) {
        if (str_casecmp_portable(local.books[i].genre, "Computer Science") == 0) {
            cs++;
        } else if (str_casestr_portable(local.books[i].genre, "Fiction")) {
            fic++;
        } else {
            other++;
        }
    }
    log_event("Child-Report", "Report: %d CS, %d Fiction, %d other (of %d).",
              cs, fic, other, loaded);

    inventory_destroy(&local);
    return 0;
}

int run_process_demo(const char *data_path)
{
    if (data_path == NULL) {
        return -1;
    }

    printf("\n========================================\n");
    printf(" PROCESS (fork/wait) DEMONSTRATION\n");
    printf("========================================\n");

#if HAVE_FORK
    {
        pid_t pid_org, pid_rep;
        int   status;

        log_event("Parent", "Parent process running (pid=%ld). Forking children.",
                  (long)getpid());

        /* ---- Fork child 1: organization ---- */
        pid_org = fork();
        if (pid_org < 0) {
            log_event("Parent", "ERROR: fork() failed for organization child.");
            return -1;
        }
        if (pid_org == 0) {
            /* CHILD 1 */
            int code;
            log_event("Child-Organize",
                      "Organization child started (pid=%ld, parent=%ld).",
                      (long)getpid(), (long)getppid());
            code = child_organization(data_path);
            _exit(code);   /* _exit: do not flush parent's buffers twice */
        }

        /* ---- Fork child 2: reporting ---- */
        pid_rep = fork();
        if (pid_rep < 0) {
            log_event("Parent", "ERROR: fork() failed for reporting child.");
            /* Reap the first child before returning. */
            waitpid(pid_org, &status, 0);
            return -1;
        }
        if (pid_rep == 0) {
            /* CHILD 2 */
            int code;
            log_event("Child-Report",
                      "Reporting child started (pid=%ld, parent=%ld).",
                      (long)getpid(), (long)getppid());
            code = child_report(data_path);
            _exit(code);
        }

        /* ---- Parent: wait for both children and read exit statuses ---- */
        log_event("Parent", "Waiting for children with waitpid()...");

        if (waitpid(pid_org, &status, 0) > 0) {
            if (WIFEXITED(status)) {
                log_event("Parent", "Organization child (pid=%ld) exited, code=%d.",
                          (long)pid_org, WEXITSTATUS(status));
            }
        }
        if (waitpid(pid_rep, &status, 0) > 0) {
            if (WIFEXITED(status)) {
                log_event("Parent", "Reporting child (pid=%ld) exited, code=%d.",
                          (long)pid_rep, WEXITSTATUS(status));
            }
        }

        log_event("Parent", "All children reaped. Parent's inventory unchanged.");
        printf("========================================\n");
        printf(" PROCESS DEMONSTRATION COMPLETE\n");
        printf("========================================\n");
        return 0;
    }
#else
    /* Honest fallback: no fake processes. */
    printf("\n[NOTICE] This binary was built WITHOUT POSIX fork() support\n");
    printf("         (native MinGW does not provide the fork() system call).\n");
    printf("         No process was created, because faking one would be\n");
    printf("         academically dishonest.\n\n");
    printf("         To run the genuine fork()/wait() demonstration, build the\n");
    printf("         project under Cygwin or MSYS2 (see README.md, section\n");
    printf("         \"Windows 11 Setup\"). The thread/mutex/semaphore parts of\n");
    printf("         this program run fully on native MinGW.\n");
    printf("========================================\n");
    (void)child_organization;
    (void)child_report;
    (void)data_path;
    return 1; /* 1 => intentionally skipped due to platform */
#endif
}
