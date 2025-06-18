#include "mime_types.h"
#include <string.h>
#include <strings.h>

typedef struct {
    const char *extension;
    const char *mime_type;
} MimeTypeEntry;

static const MimeTypeEntry mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".txt", "text/plain"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".xml", "application/xml"},
    {NULL, NULL}
};

const char* mime_get_type(const char *filename) {
    if (!filename) {
        return "application/octet-stream";
    }
    
    const char *dot = strrchr(filename, '.');
    if (!dot) {
        return "application/octet-stream";
    }
    
    for (int i = 0; mime_types[i].extension; i++) {
        if (strcasecmp(dot, mime_types[i].extension) == 0) {
            return mime_types[i].mime_type;
        }
    }
    
    return "application/octet-stream";
}
