#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_FILE "/home/pi/MWPLogData/test-log.txt"

void log_test(int verbose, int log_level, int msg_level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[100];
    time_t rawtime;
    struct tm* timeinfo;
    FILE* fptr;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S] ", timeinfo);

    fptr = fopen(LOG_FILE, "a");
    if (fptr == NULL) {
        printf("%s", buffer);
        vprintf(format, args);
        va_end(args);
        return;
    }

    switch (log_level) {
        case 1:
            if (msg_level == 1 || msg_level == 2 || msg_level == 3) {
                fputs(buffer, fptr);
                vfprintf(fptr, format, args);
                if (verbose) {
                    printf("%s", buffer);
                    vprintf(format, args);
                }
            }
            else {
                fprintf(fptr, "[UNKNOWN1] ");
            }
            break;

        case 2:
            if (msg_level ==1) {}
            else if (msg_level == 2 || msg_level == 3) {
                fputs(buffer, fptr);
                vfprintf(fptr, format, args);
                if (verbose) {
                    printf("%s", buffer);
                    vprintf(format, args);
                }
            }
            else {
                fprintf(fptr, "[UNKNOWN2] ");
            }
            break;

        case 3:
            if ( msg_level == 1 || msg_level ==2 ) {}
            else if (msg_level == 3) {
                fputs(buffer, fptr);
                vfprintf(fptr, format, args);
                if (verbose) {
                    printf("%s", buffer);
                    vprintf(format, args);
                }
            }
            else {
                fprintf(fptr, "[UNKNOWN3] ");
            }
            break;

        default:
            fprintf(fptr, "[UNKNOWN4] ");
            break;
    }

    fclose(fptr);
    va_end(args);
}
