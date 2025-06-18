#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "http_parser.h"
#include "config.h"
#include "db.h"

void handle_client_request(int client_fd, const ServerConfig *config, Database *db);

#endif
