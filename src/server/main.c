#define GNU_SOURCE
#define DEFAULT_SOURCE

#include "server.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    long port = 1600;
    if (argc >= 2)
    {
        port = strtol(argv[1], NULL, 10);
    }
    else
    {
        printf("Set default port: %ld\n", port);
        printf("Usage: srv <port>\n");
    }
    server(port);
}