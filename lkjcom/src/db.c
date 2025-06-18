#include "db.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static BTreeNode* btree_create_node(int is_leaf) {
    BTreeNode *node = safe_malloc(sizeof(BTreeNode));
    memset(node, 0, sizeof(BTreeNode));
    node->is_leaf = is_leaf;
    node->key_count = 0;
    return node;
}

static void btree_insert_non_full(BTreeNode *node, const Event *event) {
    int i = node->key_count - 1;
    
    if (node->is_leaf) {
        // Insert into leaf node
        while (i >= 0 && strcmp(event->event_id, node->keys[i]) < 0) {
            strcpy(node->keys[i + 1], node->keys[i]);
            node->events[i + 1] = node->events[i];
            i--;
        }
        strcpy(node->keys[i + 1], event->event_id);
        node->events[i + 1] = *event;
        node->key_count++;
    } else {
        // Find child to insert into
        while (i >= 0 && strcmp(event->event_id, node->keys[i]) < 0) {
            i--;
        }
        i++;
        
        // If child is full, split it
        if (node->children[i]->key_count == BTREE_ORDER - 1) {
            // Split child (simplified implementation)
            // In a full implementation, this would split the child node
        }
        
        btree_insert_non_full(node->children[i], event);
    }
}

static Event* btree_search(BTreeNode *node, const char *event_id) {
    if (!node) return NULL;
    
    int i = 0;
    while (i < node->key_count && strcmp(event_id, node->keys[i]) > 0) {
        i++;
    }
    
    if (i < node->key_count && strcmp(event_id, node->keys[i]) == 0) {
        return &node->events[i];
    }
    
    if (node->is_leaf) {
        return NULL;
    }
    
    return btree_search(node->children[i], event_id);
}

static void btree_free(BTreeNode *node) {
    if (!node) return;
    
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            btree_free(node->children[i]);
        }
    }
    free(node);
}

int db_init(Database *db, const char *db_path) {
    if (!db || !db_path) return -1;
    
    memset(db, 0, sizeof(Database));
    safe_strcpy(db->db_path, db_path, sizeof(db->db_path));
    
    db->root = btree_create_node(1); // Start with leaf node
    db->initialized = 1;
    
    // Try to load existing data
    db_load_from_file(db);
    
    log_info("Database initialized: %s", db_path);
    return 0;
}

int db_add_event(Database *db, const Event *event) {
    if (!db || !event || !db->initialized) return -1;
    
    if (db->root->key_count == BTREE_ORDER - 1) {
        // Root is full, create new root
        BTreeNode *new_root = btree_create_node(0);
        new_root->children[0] = db->root;
        db->root = new_root;
        
        // Split old root (simplified)
        // In a full implementation, this would properly split the node
    }
    
    btree_insert_non_full(db->root, event);
    
    // Save to file
    db_save_to_file(db);
    
    log_debug("Added event: %s", event->event_id);
    return 0;
}

Event* db_find_event_by_id(Database *db, const char *event_id) {
    if (!db || !event_id || !db->initialized) return NULL;
    
    return btree_search(db->root, event_id);
}

int db_delete_event_by_id(Database *db, const char *event_id) {
    // Simplified implementation - in practice would need proper B-tree deletion
    log_warn("Event deletion not fully implemented");
    return -1;
}

int db_save_to_file(Database *db) {
    if (!db || !db->initialized) return -1;
    
    FILE *file = fopen(db->db_path, "w");
    if (!file) {
        log_error("Cannot open database file for writing: %s", db->db_path);
        return -1;
    }
    
    // Simple serialization - save all events from leaf nodes
    // In practice, would traverse the entire tree
    if (db->root && db->root->is_leaf) {
        for (int i = 0; i < db->root->key_count; i++) {
            fprintf(file, "%s|%s|%ld\n", 
                   db->root->keys[i],
                   db->root->events[i].content,
                   db->root->events[i].timestamp);
        }
    }
    
    fclose(file);
    return 0;
}

int db_load_from_file(Database *db) {
    if (!db || !db->initialized) return -1;
    
    FILE *file = fopen(db->db_path, "r");
    if (!file) {
        log_info("Database file not found, starting with empty database");
        return 0;
    }
    
    char line[4096];
    while (fgets(line, sizeof(line), file)) {
        char *id = strtok(line, "|");
        char *content = strtok(NULL, "|");
        char *timestamp_str = strtok(NULL, "|\n");
        
        if (id && content && timestamp_str) {
            Event event;
            safe_strcpy(event.event_id, id, sizeof(event.event_id));
            safe_strcpy(event.content, content, sizeof(event.content));
            event.timestamp = atol(timestamp_str);
            
            btree_insert_non_full(db->root, &event);
        }
    }
    
    fclose(file);
    log_info("Loaded events from database file");
    return 0;
}

void db_close(Database *db) {
    if (!db || !db->initialized) return;
    
    db_save_to_file(db);
    btree_free(db->root);
    db->root = NULL;
    db->initialized = 0;
    
    log_info("Database closed");
}
