#include "client.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int port = 1600;
    if (argc < 2)
    {
        fprintf(stderr, "Usage: clnt <server name> <port>\n");
        exit(1);
    }
    int name_size = strlen(argv[1]) + 1;
    char name[name_size];
    strcpy(name, argv[1]);
    if (argc >= 3)
    {
        port = strtol(argv[2], NULL, 10);
    }
    else
    {
        printf("Set default port: %d\n", port);
        printf("Usage: clnt <server name> <port>\n");
    }
    client(name, port);
}