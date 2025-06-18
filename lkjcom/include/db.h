#ifndef DB_H
#define DB_H

#include "json_parser.h"

#define BTREE_ORDER 5

typedef struct BTreeNode {
    char keys[BTREE_ORDER - 1][MAX_EVENT_ID_LEN];
    Event events[BTREE_ORDER - 1];
    struct BTreeNode *children[BTREE_ORDER];
    int key_count;
    int is_leaf;
} BTreeNode;

typedef struct {
    BTreeNode *root;
    char db_path[1024];
    int initialized;
} Database;

int db_init(Database *db, const char *db_path);
int db_add_event(Database *db, const Event *event);
Event* db_find_event_by_id(Database *db, const char *event_id);
int db_delete_event_by_id(Database *db, const char *event_id);
int db_save_to_file(Database *db);
int db_load_from_file(Database *db);
void db_close(Database *db);

#endif
