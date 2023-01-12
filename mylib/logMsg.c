#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_FILE "../console-log.txt"

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[100];
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S] ", timeinfo);

    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file != NULL) {
        fputs(buffer, log_file);
        vfprintf(log_file, format, args);
        fclose(log_file);
    } else {
        printf("%s", buffer);
        vprintf(format, args);
    }

    va_end(args);
}