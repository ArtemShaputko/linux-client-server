#define _GNU_SOURCE
#define DEFAULT_SOURCE

#include "client.h"
#include "../utils/structs.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <linux/limits.h>
#include <fcntl.h>

char srv_dir[200] = {0};

int client(const char *server_name, int port)
{
    int s;
    struct sockaddr_in serv;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket:");
        return 1;
    }

    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    serv.sin_addr.s_addr = inet_addr(server_name);

    if (connect(s, (struct sockaddr *)&serv, sizeof(serv)) < 0)
    {
        perror("connect:");
        return 1;
    }
    printf("READY\n");
    int res = handle_keyboard(s);
    close(s);

    return res;
}

int handle_keyboard(int serv_socket)
{
    int send_res;
    while (1)
    {
        send_res = send_message(stdin, serv_socket);
        if (send_res > 0)
            return 0;
        else if (send_res < 0)
            return 1;
    }
    return 0;
}

int handle_file(const char *filename, const int socket)
{
    FILE *file = fopen(filename, "r");
    int send_res;
    if (file == NULL)
    {
        fprintf(stderr, "Cannot open file\n");
        return 1;
    }
    while (!feof(file))
    {
        send_res = send_message(file, socket);
        if (send_res > 0)
            return 0;
        else if (send_res < 0)
            return 1;
    }
    return 0;
}

int send_message(FILE *file, int serv_socket)
{
    msg_t read_msg, write_msg;
    int readed;
    memset(&write_msg, 0, sizeof(msg_t));
    memset(&read_msg, 0, sizeof(msg_t));
    printf("%s> ", srv_dir);
    fgets(write_msg.buffer, BUF_SIZE, file);
    if (file != stdin)
        printf("%s", write_msg.buffer);
    if (write_msg.buffer[strlen(write_msg.buffer) - 1] == '\n')
    {
        write_msg.buffer[strlen(write_msg.buffer) - 1] = '\0';
    }
    if (strcasecmp(write_msg.buffer, "QUIT") == 0)
    {
        printf("EXIT\n");
        return 1;
    }
    else if (write_msg.buffer[0] == '@')
    {
        if (handle_file(&write_msg.buffer[1], serv_socket) > 0)
            fprintf(stderr, "Detected errors, reading stopped\n");
        else
            printf("Reading file stopped\n");
    }
    else
    {
        get_time(write_msg.time, TIME_SIZE);
        write_msg.is_error = 0;
        write(serv_socket, &write_msg, sizeof(msg_t));
        readed = read(serv_socket, &read_msg, sizeof(msg_t));
        if (readed == 0)
        {
            printf("Server stopped\n");
            return -1;
        }
        else if (readed < 0)
        {
            fprintf(stderr, "Detected errors while recievig data!\n");
            return 0;
        }
        FILE *stream = read_msg.is_error ? stderr : stdout;
        fprintf(stream, "%s", read_msg.buffer);
        strcpy(srv_dir, read_msg.prompt);
    }
    return 0;
}