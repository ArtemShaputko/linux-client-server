#include "../utils/structs.h"

#include <pthread.h>
#include <unistd.h>
#include <dirent.h>

typedef struct
{
    pthread_t *threads;
    size_t max;
    size_t count;
} thread_array;

int server(const int port);
void *handle_client(void *data);
void close_threads(void *threads);
void close_handler(int sig);
void handle_echo(const int socket, const char *cur_dir, const char *params);
void write_message(const int socket, const char *cur_dir, const char *buffer, int is_error);
void handle_info(const int socket, const char *cur_dir);
void handle_list(const int socket, const char *cur_dir);
int is_below(char *path1, char *path2);
void handle_cd(const int socket, char *cur_dir, const char *params);
int check_dir(char *path);
int is_equal(char *path1, char *path2);
void parse_link(char *buffer, struct dirent *dent);
void logger(msg_t msg, int is_request);