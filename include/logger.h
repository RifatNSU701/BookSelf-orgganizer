#ifndef LOGGER_H
#define LOGGER_H

int logger_init(const char *logfile);
void logger_close(void);
void log_event(const char *tag, const char *fmt, ...);

#endif
