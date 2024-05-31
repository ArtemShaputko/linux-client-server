#define _GNU_SOURCE

#include "server.h"
#include "../utils/structs.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <linux/limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <stddef.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define INFO_FILE_NAME "info"
thread_array clients = {0};
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
char thread_name[50];

void close_handler(int sig)
{
    close_threads((void *)&clients);
    signal(sig, SIG_DFL);
}

int server(const int port)
{
    int bind_socket, accept_socket;
    socklen_t clen;
    struct sockaddr_in serv = {0}, client = {0};
    clients.count = 0;
    clients.max = 10;

    if ((bind_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket:");
        return 1;
    }
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);

    if ((bind(bind_socket, (struct sockaddr *)&serv,
              sizeof(struct sockaddr_in))) < 0)
    {
        perror("bind:");
        return 1;
    }
    if ((listen(bind_socket, 10)) < 0)
    {
        perror("listen:");
        return 1;
    }
    printf("READY\n");
    clients.threads = calloc(clients.max, sizeof(pthread_t));
    on_exit((void *)&close_threads, &clients);
    signal(SIGKILL, close_handler);
    while (1)
    {
        memset(&client, 0, sizeof(client));
        clen = sizeof(struct sockaddr_in);
        if ((accept_socket = accept(bind_socket, (struct sockaddr *)&client,
                                    &clen)) < 0)
        {
            perror("accept:");
            continue;
        }
        if (pthread_create(&clients.threads[clients.count], NULL, handle_client, &accept_socket) != 0)
        {
            perror("thread_create");
            continue;
        }
        snprintf(thread_name, 50, "Client_%ld", clients.count + 1);
        pthread_setname_np(clients.threads[clients.count], thread_name);
        clients.count++;
        if (clients.count == clients.max)
        {
            clients.max *= 2;
            clients.threads = realloc(clients.threads, sizeof(pthread_t) * clients.max);
        }
    }
}

void *handle_client(void *data)
{
    int socket = *(int *)data;
    msg_t read_msg;
    char *command;
    char *params;
    char cur_dir[PROMPT_SIZE] = {0};
    unshare(CLONE_FS);

    while (1)
    {
        memset(&read_msg, 0, sizeof(msg_t));
        if (read(socket, &read_msg, sizeof(read_msg)) <= 0)
        {
            close(socket);
            return 0;
        }
        logger(read_msg, 1);
        command = strtok(read_msg.buffer, " ");
        if (command == NULL)
        {
            close(socket);
            return 0;
        }
        params = strtok(NULL, "\0");
        if (strcmp(command, "ECHO") == 0)
        {
            handle_echo(socket, cur_dir, params);
        }
        else if (strcmp(command, "INFO") == 0)
        {
            handle_info(socket, cur_dir);
        }
        else if (strcmp(command, "LIST") == 0)
        {
            handle_list(socket, cur_dir);
        }
        else if (strcmp(command, "CD") == 0)
        {
            handle_cd(socket, cur_dir, params);
        }
        else
        {
            write_message(socket, cur_dir, "Wrong command!\n", 1);
            continue;
        }
    }
}

void close_threads(void *threads)
{
    thread_array *array = (thread_array *)threads;
    if (array->threads == NULL)
        return;
    for (size_t i = 0; i < array->count; i++)
    {
        pthread_cancel(array->threads[i]);
        pthread_join(array->threads[i], NULL);
    }
    free(array->threads);
}

void handle_echo(const int socket, const char *cur_dir, const char *params)
{
    char buffer[BUF_SIZE];
    if (params == NULL)
        memset(buffer, 0, BUF_SIZE);
    else
        sprintf(buffer, "%s\n", params);
    write_message(socket, cur_dir, buffer, 0);
}

void write_message(const int socket, const char *cur_dir, const char *buffer, int is_error)
{
    msg_t write_msg = {0};
    strcpy(write_msg.prompt, cur_dir);
    strcpy(write_msg.buffer, buffer);
    get_time(write_msg.time, TIME_SIZE);
    write_msg.is_error = is_error;
    write(socket, &write_msg, sizeof(msg_t));
    logger(write_msg, 0);
}

void handle_info(const int socket, const char *cur_dir)
{
    int fd = open(INFO_FILE_NAME, O_CREAT | O_RDWR);
    if (fd < 0)
    {
        write_message(socket, cur_dir, "Cannot open info file\n", 1);
        return;
    }
    char buffer[BUF_SIZE] = {0};
    read(fd, buffer, BUF_SIZE);
    write_message(socket, cur_dir, buffer, 0);
    close(fd);
}

void handle_list(const int socket, const char *cur_dir)
{
    char buffer[BUF_SIZE] = {0};
    char dir_path[PATH_MAX];
    struct dirent *dent;
    sprintf(dir_path, "./%s", cur_dir);
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        write_message(socket, cur_dir, "Cannot open folder\n", 0);
        return;
    }
    while ((dent = readdir(dir)) != NULL)
    {
        strcat(buffer, dent->d_name);
        if (dent->d_type == DT_DIR)
            strcat(buffer, "/");
        else if (dent->d_type == DT_LNK)
        {
            parse_link(buffer, dent);
        }
        strcat(buffer, "\n");
    }
    closedir(dir);
    write_message(socket, cur_dir, buffer, 0);
}

void parse_link(char *buffer, struct dirent *dent)
{
    struct stat st;
    strcat(buffer, " -->");
    char linkpath[BUF_SIZE];
    ssize_t len = readlink(dent->d_name, linkpath, sizeof(linkpath) - 1);

    linkpath[len] = '\0';

    if (lstat(linkpath, &st) == 0 && S_ISLNK(st.st_mode))
    {
        strcat(buffer, "> ");
    }
    else
    {
        strcat(buffer, " ");
    }

    if (!is_below(linkpath, "."))
    {
        strcpy(linkpath, "External");
    }
    strcat(buffer, linkpath);
}

void logger(msg_t msg, int is_request)
{
    char name[16];
    pthread_t current_thread = pthread_self();
    pthread_getname_np(current_thread, name, 16);

    pthread_mutex_lock(&log_mutex);

    printf("%s | %s ", msg.time, name);
    printf(is_request ? "recieve\n" : "send\n");
    printf("\tError: %d\n", msg.is_error);
    printf("\tText:\n%s\n", msg.buffer);
    printf("\tPrompt: %s\n", msg.prompt);

    pthread_mutex_unlock(&log_mutex);
}

void handle_cd(const int socket, char *cur_prompt, const char *params)
{
    char new_prompt[PROMPT_SIZE];
    if (strlen(cur_prompt) != 0)
        sprintf(new_prompt, "%s/%s", cur_prompt, params);
    else
        sprintf(new_prompt, "%s", params);
    char new_dir[PATH_MAX];
    sprintf(new_dir, "./%s", new_prompt);
    int err = !is_below(new_dir, ".");
    if (err != 0)
    {
        char buffer[BUF_SIZE];
        if (err > 0)
            sprintf(buffer, "Directory %s is above the root\n", new_prompt);
        else
            sprintf(buffer, "Directory %s not found\n", new_prompt);
        write_message(socket, cur_prompt, buffer, 1);
        return;
    }
    if (!check_dir(new_dir))
    {
        write_message(socket, cur_prompt, "Wrong directory\n", 1);
        return;
    }
    strcpy(cur_prompt, new_prompt);
    if (is_equal(new_dir, ".") == 1)
    {
        memset(cur_prompt, 0, PROMPT_SIZE);
    }
    write_message(socket, cur_prompt, "", 0);
}

int check_dir(char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
        return 0;
    return 1;
}

int is_below(char *path1, char *path2)
{
    char resolved1[PATH_MAX];
    char resolved2[PATH_MAX];

    if (realpath(path1, resolved1) == NULL)
    {
        return -1;
    }
    if (realpath(path2, resolved2) == NULL)
    {
        return -1;
    }

    return strstr(resolved1, resolved2) != NULL;
}

int is_equal(char *path1, char *path2)
{
    char resolved1[PATH_MAX];
    char resolved2[PATH_MAX];

    if (realpath(path1, resolved1) == NULL)
    {
        return -1;
    }
    if (realpath(path2, resolved2) == NULL)
    {
        return -1;
    }

    return strcmp(resolved1, resolved2) == 0;
}