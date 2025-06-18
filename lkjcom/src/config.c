#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void config_set_defaults(ServerConfig *config) {
    config->port = DEFAULT_PORT;
    safe_strcpy(config->document_root, DEFAULT_DOC_ROOT, sizeof(config->document_root));
    safe_strcpy(config->database_path, DEFAULT_DB_PATH, sizeof(config->database_path));
    safe_strcpy(config->host, "0.0.0.0", sizeof(config->host));
}

int config_parse_args(int argc, char *argv[], ServerConfig *config) {
    config_set_defaults(config);
    
    if (argc > 1) {
        config->port = atoi(argv[1]);
        if (config->port <= 0 || config->port > 65535) {
            log_error("Invalid port number: %s", argv[1]);
            return -1;
        }
    }
    
    if (argc > 2) {
        safe_strcpy(config->document_root, argv[2], sizeof(config->document_root));
    }
    
    if (argc > 3) {
        safe_strcpy(config->database_path, argv[3], sizeof(config->database_path));
    }
    
    log_info("Server configuration:");
    log_info("  Port: %d", config->port);
    log_info("  Document Root: %s", config->document_root);
    log_info("  Database Path: %s", config->database_path);
    
    return 0;
}
