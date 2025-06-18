#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_PORT 8080
#define DEFAULT_DOC_ROOT "./routes"
#define DEFAULT_DB_PATH "./events.db"
#define MAX_PATH_LEN 1024
#define MAX_HOST_LEN 256

typedef struct {
    int port;
    char document_root[MAX_PATH_LEN];
    char database_path[MAX_PATH_LEN];
    char host[MAX_HOST_LEN];
} ServerConfig;

int config_parse_args(int argc, char *argv[], ServerConfig *config);
void config_set_defaults(ServerConfig *config);

#endif
