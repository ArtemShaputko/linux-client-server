#include <stdio.h>

int client(const char *server_name, int port);
int handle_keyboard(int serv_socket);
int send_message(FILE *file, int serv_socket);
int handle_file(const char *filename, const int socket);