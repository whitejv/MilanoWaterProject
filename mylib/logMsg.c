#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_FILE "/home/pi/MWPLogData/console-log.txt"

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[100];
    time_t rawtime;
    struct tm *timeinfo;
    FILE *fptr;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S] ", timeinfo);

    fptr = fopen(LOG_FILE, "a");
    if (fptr != NULL) {
        fputs(buffer, fptr);
        vfprintf(fptr, format, args);
        fclose(fptr);
    } else {
        printf("%s", buffer);
        vprintf(format, args);
    }

    va_end(args);
}