#ifndef STRUCTS_H
#define STRUCTS_H

#include <stddef.h>

#define BUF_SIZE 1024
#define PROMPT_SIZE 200
#define TIME_SIZE 24

void get_time(char *buffer, size_t size);

typedef struct
{
    char time[TIME_SIZE];
    char buffer[BUF_SIZE];
    char prompt[PROMPT_SIZE];
    unsigned is_error;
} msg_t;

#endif