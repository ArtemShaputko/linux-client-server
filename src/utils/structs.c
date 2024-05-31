#include "structs.h"

#include <sys/time.h>
#include <stdio.h>
#include <time.h>

void get_time(char *buffer, size_t size)
{
    time_t rawtime;
    struct tm *timeinfo;
    struct timeval tv;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    gettimeofday(&tv, NULL);
    strftime(buffer, size, "%Y.%m.%d-%H:%M:%S.", timeinfo);
    snprintf(buffer + 19, size - 19, "%03lu", (long)tv.tv_usec / 1000);
}