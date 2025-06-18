#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#define MAX_EVENT_ID_LEN 64
#define MAX_EVENT_CONTENT_LEN 4096
#define MAX_EVENTS_PER_REQUEST 100

typedef struct {
    char event_id[MAX_EVENT_ID_LEN];
    char content[MAX_EVENT_CONTENT_LEN];
    long timestamp;
} Event;

typedef struct {
    Event *events;
    int count;
    int capacity;
} EventList;

EventList* json_parse_events(const char *json_data);
char* json_serialize_events(const EventList *events);
void event_list_free(EventList *list);

#endif
