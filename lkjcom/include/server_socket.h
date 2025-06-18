#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

int server_socket_create(int port);
int server_socket_accept(int server_fd);
void server_socket_close(int socket_fd);

#endif
