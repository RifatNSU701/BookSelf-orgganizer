#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>   /* getpid */

static FILE           *g_logfp = NULL;
static pthread_mutex_t g_log_lock;
static int             g_log_ready = 0;

int logger_init(const char *logfile)
{
    if (pthread_mutex_init(&g_log_lock, NULL) != 0) {
        return -1;
    }
    /* Append mode so multiple runs (and forked children) accumulate history. */
    g_logfp = fopen(logfile, "a");
    if (g_logfp == NULL) {
        /* Not fatal: fall back to stderr only, but keep the mutex valid. */
        g_logfp = NULL;
    }
    g_log_ready = 1;
    return 0;
}

void logger_close(void)
{
    if (!g_log_ready) {
        return;
    }
    pthread_mutex_lock(&g_log_lock);
    if (g_logfp != NULL) {
        fclose(g_logfp);
        g_logfp = NULL;
    }
    pthread_mutex_unlock(&g_log_lock);
    pthread_mutex_destroy(&g_log_lock);
    g_log_ready = 0;
}

void log_event(const char *tag, const char *fmt, ...)
{
    va_list ap;
    long pid;
    unsigned long tid;

    if (!g_log_ready) {
        return;
    }

    /* Real OS identifiers so the concurrency is genuinely observable. */
    pid = (long)getpid();
    /* pthread_self() is opaque; cast to an unsigned long for a short handle. */
    tid = (unsigned long)(pthread_self());

    pthread_mutex_lock(&g_log_lock);

    /* Write to console. */
    printf("[PID=%ld][TID=%lu][%s] ", pid, tid % 100000UL, tag);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);

    /* Mirror to the log file if available. */
    if (g_logfp != NULL) {
        fprintf(g_logfp, "[PID=%ld][TID=%lu][%s] ", pid, tid % 100000UL, tag);
        va_start(ap, fmt);
        vfprintf(g_logfp, fmt, ap);
        va_end(ap);
        fprintf(g_logfp, "\n");
        fflush(g_logfp);
    }

    pthread_mutex_unlock(&g_log_lock);
}
