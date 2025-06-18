#include "server_socket.h"
#include "utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int server_socket_create(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_error("Socket creation failed: %s", strerror(errno));
        return -1;
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        log_error("setsockopt failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_error("Bind failed on port %d: %s", port, strerror(errno));
        close(server_fd);
        return -1;
    }
    
    // Listen
    if (listen(server_fd, 3) < 0) {
        log_error("Listen failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    log_info("Server listening on port %d", port);
    return server_fd;
}

int server_socket_accept(int server_fd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int client_fd;
    
    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (client_fd < 0) {
        log_error("Accept failed: %s", strerror(errno));
        return -1;
    }
    
    log_debug("Accepted connection from %s:%d", 
              inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    
    return client_fd;
}

void server_socket_close(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}
