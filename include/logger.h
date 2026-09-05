#ifndef LOGGER_H
#define LOGGER_H

/*
 * logger.h
 * A small thread-safe logging facility. Every log line is stamped with the
 * real process id (getpid) and a short thread tag, so the concurrency in the
 * program is actually visible on screen and in logs/session.log.
 *
 * The logger itself uses a dedicated mutex so that concurrent log calls from
 * many threads do not interleave mid-line.
 */

/* Initialise the logger, opening 'logfile' for appending. Returns 0 on success. */
int  logger_init(const char *logfile);

/* Flush and close the logger, destroying its internal mutex. */
void logger_close(void);

/*
 * log_event
 * printf-style logging. 'tag' is a short label for the emitting thread/phase
 * (e.g. "Alphabetical", "Inventory", "SEMAPHORE", "MUTEX"). The real pid and
 * a truncated thread id are prepended automatically.
 */
void log_event(const char *tag, const char *fmt, ...);

#endif /* LOGGER_H */
