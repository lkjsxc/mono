#include "config.h"
#include "server_socket.h"
#include "request_handler.h"
#include "db.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static volatile int server_running = 1;
static int server_fd = -1;
static Database db;

void signal_handler(int sig) {
    log_info("Received signal %d, shutting down server...", sig);
    server_running = 0;
    if (server_fd >= 0) {
        server_socket_close(server_fd);
    }
}

int main(int argc, char *argv[]) {
    ServerConfig config;
    
    // Parse configuration
    if (config_parse_args(argc, argv, &config) != 0) {
        fprintf(stderr, "Usage: %s [port] [document_root] [database_path]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize database
    if (db_init(&db, config.database_path) != 0) {
        log_error("Failed to initialize database");
        return EXIT_FAILURE;
    }
    
    // Create server socket
    server_fd = server_socket_create(config.port);
    if (server_fd < 0) {
        log_error("Failed to create server socket");
        db_close(&db);
        return EXIT_FAILURE;
    }
    
    log_info("HTTP Server started successfully");
    log_info("Document root: %s", config.document_root);
    log_info("Database: %s", config.database_path);
    log_info("Server running on port %d", config.port);
    
    // Main server loop
    while (server_running) {
        int client_fd = server_socket_accept(server_fd);
        if (client_fd < 0) {
            if (server_running) {
                log_error("Failed to accept client connection");
            }
            continue;
        }
        
        // Handle request (single-threaded as specified)
        handle_client_request(client_fd, &config, &db);
        
        // Close client connection
        server_socket_close(client_fd);
    }
    
    // Cleanup
    server_socket_close(server_fd);
    db_close(&db);
    log_info("Server shutdown complete");
    
    return EXIT_SUCCESS;
}
