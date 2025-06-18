#include "json_parser.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Simple JSON parser - basic implementation
EventList* json_parse_events(const char *json_data) {
    if (!json_data) return NULL;
    
    EventList *list = safe_malloc(sizeof(EventList));
    list->capacity = 10;
    list->count = 0;
    list->events = safe_malloc(sizeof(Event) * list->capacity);
    
    // Very basic JSON parsing - looking for event objects
    const char *ptr = json_data;
    
    while (*ptr) {
        // Find start of event object
        char *event_start = strstr(ptr, "{");
        if (!event_start) break;
        
        // Find end of event object
        char *event_end = strstr(event_start + 1, "}");
        if (!event_end) break;
        
        // Extract event data
        size_t event_len = event_end - event_start + 1;
        char *event_json = safe_malloc(event_len + 1);
        memcpy(event_json, event_start, event_len);
        event_json[event_len] = '\0';
        
        // Parse event fields (basic implementation)
        Event event;
        memset(&event, 0, sizeof(Event));
        
        // Look for event_id
        char *id_start = strstr(event_json, "\"event_id\"");
        if (id_start) {
            id_start = strchr(id_start, ':');
            if (id_start) {
                id_start = strchr(id_start, '"');
                if (id_start) {
                    id_start++;
                    char *id_end = strchr(id_start, '"');
                    if (id_end) {
                        size_t id_len = id_end - id_start;
                        if (id_len < MAX_EVENT_ID_LEN) {
                            memcpy(event.event_id, id_start, id_len);
                            event.event_id[id_len] = '\0';
                        }
                    }
                }
            }
        }
        
        // Look for content
        char *content_start = strstr(event_json, "\"content\"");
        if (content_start) {
            content_start = strchr(content_start, ':');
            if (content_start) {
                content_start = strchr(content_start, '"');
                if (content_start) {
                    content_start++;
                    char *content_end = strchr(content_start, '"');
                    if (content_end) {
                        size_t content_len = content_end - content_start;
                        if (content_len < MAX_EVENT_CONTENT_LEN) {
                            memcpy(event.content, content_start, content_len);
                            event.content[content_len] = '\0';
                        }
                    }
                }
            }
        }
        
        event.timestamp = time(NULL);
        
        // Add event to list
        if (list->count >= list->capacity) {
            list->capacity *= 2;
            list->events = safe_realloc(list->events, sizeof(Event) * list->capacity);
        }
        
        if (strlen(event.event_id) > 0) {
            list->events[list->count++] = event;
        }
        
        free(event_json);
        ptr = event_end + 1;
    }
    
    return list;
}

char* json_serialize_events(const EventList *events) {
    if (!events || events->count == 0) {
        return safe_strdup("[]");
    }
    
    // Estimate size needed
    size_t estimated_size = 1024 + (events->count * 512);
    char *json = safe_malloc(estimated_size);
    
    strcpy(json, "[");
    
    for (int i = 0; i < events->count; i++) {
        char event_json[1024];
        snprintf(event_json, sizeof(event_json),
                "%s{\"event_id\":\"%s\",\"content\":\"%s\",\"timestamp\":%ld}",
                (i > 0) ? "," : "",
                events->events[i].event_id,
                events->events[i].content,
                events->events[i].timestamp);
        
        strcat(json, event_json);
    }
    
    strcat(json, "]");
    return json;
}

void event_list_free(EventList *list) {
    if (list) {
        if (list->events) {
            free(list->events);
        }
        free(list);
    }
}
