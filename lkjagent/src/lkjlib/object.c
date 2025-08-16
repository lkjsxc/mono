#include "lkjlib.h"

// Object
static result_t escape_json_data(pool_t* pool, const data_t* input, data_t** output) {
    uint64_t cap = (input ? input->size : 0) * 2 + 2;
    if (pool_data_alloc(pool, output, cap) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer for JSON-escaped string");
    }
    (*output)->size = 0;
    if (!input || input->size == 0)
        return RESULT_OK;
    for (uint64_t i = 0; i < input->size; i++) {
        unsigned char ch = (unsigned char)input->data[i];
        switch (ch) {
            case '"':
                if (data_append_str(pool, output, "\\\"") != RESULT_OK) {
                    RETURN_ERR("Failed to append escaped quote while escaping JSON string");
                }
                break;
            case '\\':
                if (data_append_str(pool, output, "\\\\") != RESULT_OK) {
                    RETURN_ERR("Failed to append escaped backslash while escaping JSON string");
                }
                break;
            case '\b':
                if (data_append_str(pool, output, "\\b") != RESULT_OK) {
                    RETURN_ERR("Failed to append \\b while escaping JSON string");
                }
                break;
            case '\f':
                if (data_append_str(pool, output, "\\f") != RESULT_OK) {
                    RETURN_ERR("Failed to append \\f while escaping JSON string");
                }
                break;
            case '\n':
                if (data_append_str(pool, output, "\\n") != RESULT_OK) {
                    RETURN_ERR("Failed to append \\n while escaping JSON string");
                }
                break;
            case '\r':
                if (data_append_str(pool, output, "\\r") != RESULT_OK) {
                    RETURN_ERR("Failed to append \\r while escaping JSON string");
                }
                break;
            case '\t':
                if (data_append_str(pool, output, "\\t") != RESULT_OK) {
                    RETURN_ERR("Failed to append \\t while escaping JSON string");
                }
                break;
            default:
                if (ch < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", ch);
                    if (data_append_str(pool, output, buf) != RESULT_OK) {
                        RETURN_ERR("Failed to append Unicode escape while escaping JSON string");
                    }
                } else {
                    if (data_append_char(pool, output, (char)ch) != RESULT_OK) {
                        RETURN_ERR("Failed to append raw character while escaping JSON string");
                    }
                }
        }
    }
    return RESULT_OK;
}

static const char* skip_ws(const char* p, const char* end) {
    while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
        p++;
    return p;
}

static result_t parse_json_data(pool_t* pool, const char** json, const char* end, data_t** out) {
    const char* p = *json;
    if (p >= end) {
        RETURN_ERR("Unexpected end of input while parsing JSON string");
    }
    if (*p != '"') {
        RETURN_ERR("Expected opening double quote to start JSON string");
    }
    p++;
    const char* start = p;
    while (p < end) {
        if (*p == '"')
            break;
        if (*p == '\\' && p + 1 < end) {
            p += 2;
            continue;
        }
        p++;
    }
    if (p >= end || *p != '"') {
        RETURN_ERR("Unterminated JSON string literal");
    }
    size_t raw_len = (size_t)(p - start);
    if (pool_data_alloc(pool, out, raw_len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer for raw JSON string");
    }
    (*out)->size = raw_len;
    memcpy((*out)->data, start, raw_len);
    data_t* decoded = NULL;
    if (pool_data_alloc(pool, &decoded, raw_len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer for decoded JSON string");
    }
    decoded->size = 0;
    for (size_t i = 0; i < raw_len;) {
        char c = (*out)->data[i];
        if (c == '\\' && i + 1 < raw_len) {
            char e = (*out)->data[i + 1];
            switch (e) {
                case '"':
                    if (data_append_char(pool, &decoded, '"') != RESULT_OK) {
                        RETURN_ERR("Failed to append double quote while decoding JSON escape");
                    }
                    break;
                case '\\':
                    if (data_append_char(pool, &decoded, '\\') != RESULT_OK) {
                        RETURN_ERR("Failed to append backslash while decoding JSON escape");
                    }
                    break;
                case '/':
                    if (data_append_char(pool, &decoded, '/') != RESULT_OK) {
                        RETURN_ERR("Failed to append '/' while decoding JSON escape");
                    }
                    break;
                case 'b':
                    if (data_append_char(pool, &decoded, '\b') != RESULT_OK) {
                        RETURN_ERR("Failed to append backspace while decoding JSON escape");
                    }
                    break;
                case 'f':
                    if (data_append_char(pool, &decoded, '\f') != RESULT_OK) {
                        RETURN_ERR("Failed to append form-feed while decoding JSON escape");
                    }
                    break;
                case 'n':
                    if (data_append_char(pool, &decoded, '\n') != RESULT_OK) {
                        RETURN_ERR("Failed to append newline while decoding JSON escape");
                    }
                    break;
                case 'r':
                    if (data_append_char(pool, &decoded, '\r') != RESULT_OK) {
                        RETURN_ERR("Failed to append carriage return while decoding JSON escape");
                    }
                    break;
                case 't':
                    if (data_append_char(pool, &decoded, '\t') != RESULT_OK) {
                        RETURN_ERR("Failed to append tab while decoding JSON escape");
                    }
                    break;
                default:
                    if (data_append_char(pool, &decoded, e) != RESULT_OK) {
                        RETURN_ERR("Invalid escape sequence in JSON string");
                    }
                    break;
            }
            i += 2;
        } else {
            if (data_append_char(pool, &decoded, c) != RESULT_OK) {
                RETURN_ERR("Failed to append character to decoded JSON string");
            }
            i++;
        }
    }
    data_t* tmp = *out;
    *out = decoded;
    if (pool_data_free(pool, tmp) != RESULT_OK)
        RETURN_ERR("Failed to free temporary buffer for raw JSON string");
    *json = p + 1;
    return RESULT_OK;
}

static result_t parse_json_value_local(pool_t* pool, const char** json, const char* end, object_t** out);

static result_t parse_json_array_local(pool_t* pool, const char** json, const char* end, object_t** out) {
    const char* p = *json;
    if (p >= end || *p != '[') {
        RETURN_ERR("Expected '[' at start of JSON array");
    }
    p++;
    p = skip_ws(p, end);
    if (pool_object_alloc(pool, out) != RESULT_OK) {
        RETURN_ERR("Failed to allocate array object from pool");
    }
    (*out)->data = NULL;
    (*out)->child = NULL;
    (*out)->next = NULL;
    object_t* first = NULL;
    object_t* last = NULL;
    if (p < end && *p == ']') {
        *json = p + 1;
        return RESULT_OK;
    }
    while (p < end) {
        object_t* elem;
        if (parse_json_value_local(pool, &p, end, &elem) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON array element");
        }
        if (!first) {
            first = last = elem;
        } else {
            last->next = elem;
            last = elem;
        }
        p = skip_ws(p, end);
        if (p < end && *p == ',') {
            p++;
            p = skip_ws(p, end);
            continue;
        }
        if (p < end && *p == ']')
            break;
        RETURN_ERR("Expected ',' or ']' while parsing JSON array");
    }
    if (p >= end || *p != ']') {
        RETURN_ERR("Unterminated JSON array");
    }
    (*out)->child = first;
    *json = p + 1;
    return RESULT_OK;
}

static result_t parse_json_object_local(pool_t* pool, const char** json, const char* end, object_t** out) {
    const char* p = *json;
    if (p >= end || *p != '{') {
        RETURN_ERR("Expected '{' at start of JSON object");
    }
    p++;
    p = skip_ws(p, end);
    if (pool_object_alloc(pool, out) != RESULT_OK) {
        RETURN_ERR("Failed to allocate object from pool");
    }
    (*out)->data = NULL;
    (*out)->child = NULL;
    (*out)->next = NULL;
    object_t* first = NULL;
    object_t* last = NULL;
    if (p < end && *p == '}') {
        *json = p + 1;
        return RESULT_OK;
    }
    while (p < end) {
        p = skip_ws(p, end);
        data_t* key = NULL;
        if (*p != '"') {
            RETURN_ERR("Expected string key in JSON object");
        }
        if (parse_json_data(pool, &p, end, &key) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON object key");
        }
        p = skip_ws(p, end);
        if (p >= end || *p != ':') {
            RETURN_ERR("Expected ':' after object key");
        }
        p++;
        p = skip_ws(p, end);
        object_t* val;
        if (parse_json_value_local(pool, &p, end, &val) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON object value");
        }
        object_t* pair;
        if (pool_object_alloc(pool, &pair) != RESULT_OK) {
            RETURN_ERR("Failed to allocate key-value node from pool");
        }
        pair->data = key;
        pair->child = val;
        pair->next = NULL;
        if (!first) {
            first = last = pair;
        } else {
            last->next = pair;
            last = pair;
        }
        p = skip_ws(p, end);
        if (p < end && *p == ',') {
            p++;
            p = skip_ws(p, end);
            continue;
        }
        if (p < end && *p == '}')
            break;
        RETURN_ERR("Expected ',' or '}' while parsing JSON object");
    }
    if (p >= end || *p != '}') {
        RETURN_ERR("Unterminated JSON object");
    }
    (*out)->child = first;
    *json = p + 1;
    return RESULT_OK;
}

static result_t parse_primitive_local(pool_t* pool, const char** json, const char* end, data_t** out) {
    const char* p = *json;
    const char* start = p;
    while (p < end && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
        p++;
    if (p == start) {
        RETURN_ERR("Invalid JSON primitive literal");
    }
    size_t len = (size_t)(p - start);
    if (pool_data_alloc(pool, out, len) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer for JSON primitive");
    }
    (*out)->size = len;
    memcpy((*out)->data, start, len);
    *json = p;
    return RESULT_OK;
}

static result_t parse_json_value_local(pool_t* pool, const char** json, const char* end, object_t** out) {
    const char* p = skip_ws(*json, end);
    if (p >= end) {
        RETURN_ERR("Unexpected end of JSON input");
    }
    if (*p == '"') {
        if (pool_object_alloc(pool, out) != RESULT_OK) {
            RETURN_ERR("Failed to allocate object from pool");
        }
        if (parse_json_data(pool, &p, end, &((*out)->data)) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON string value");
        }
        (*out)->child = NULL;
        (*out)->next = NULL;
        *json = p;
        return RESULT_OK;
    } else if (*p == '{') {
        *json = p;
        return parse_json_object_local(pool, json, end, out);
    } else if (*p == '[') {
        *json = p;
        return parse_json_array_local(pool, json, end, out);
    } else {
        if (pool_object_alloc(pool, out) != RESULT_OK) {
            RETURN_ERR("Failed to allocate object from pool");
        }
        if (parse_primitive_local(pool, &p, end, &((*out)->data)) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON primitive value");
        }
        (*out)->child = NULL;
        (*out)->next = NULL;
        *json = p;
        return RESULT_OK;
    }
}

result_t object_create(pool_t* pool, object_t** dst) {
    if (pool_object_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate object from pool");
    }
    (*dst)->data = NULL;
    (*dst)->child = NULL;
    (*dst)->next = NULL;
    return RESULT_OK;
}

static result_t object_destroy_recursive(pool_t* pool, object_t* obj) {
    if (!obj)
        return RESULT_OK;
    if (obj->data) {
        if (data_destroy(pool, obj->data) != RESULT_OK) {
            RETURN_ERR("Failed to destroy object's data field");
        }
        obj->data = NULL;
    }
    if (obj->child) {
        object_t* c = obj->child;
        while (c) {
            object_t* nxt = c->next;
            if (object_destroy_recursive(pool, c) != RESULT_OK) {
                RETURN_ERR("Failed to destroy child object");
            }
            c = nxt;
        }
        obj->child = NULL;
    }
    if (pool_object_free(pool, obj) != RESULT_OK) {
        RETURN_ERR("Failed to return object to pool");
    }
    return RESULT_OK;
}

result_t object_destroy(pool_t* pool, object_t* object) {
    if (!object)
        return RESULT_OK;
    return object_destroy_recursive(pool, object);
}

result_t object_parse_json(pool_t* pool, object_t** dst, const data_t* src) {
    if (!src || src->size == 0) {
        RETURN_ERR("Empty JSON data");
    }
    const char* json = src->data;
    const char* end = src->data + src->size;
    const char* p = skip_ws(json, end);
    if (parse_json_value_local(pool, &p, end, dst) != RESULT_OK) {
        RETURN_ERR("Failed to parse JSON document");
    }
    return RESULT_OK;
}

static int32_t is_json_primitive_local(const data_t* s) {
    if (!s || s->size == 0)
        return 0;
    if (data_equal_str(s, "null") || data_equal_str(s, "true") || data_equal_str(s, "false"))
        return 1;
    const char* d = s->data;
    size_t n = s->size;
    size_t i = 0;
    if (d[i] == '-')
        i++;
    int32_t has_digit = 0;
    while (i < n && isdigit((unsigned char)d[i])) {
        has_digit = 1;
        i++;
    }
    if (i < n && d[i] == '.') {
        i++;
        while (i < n && isdigit((unsigned char)d[i])) {
            has_digit = 1;
            i++;
        }
    }
    if (!has_digit)
        return 0;
    if (i < n && (d[i] == 'e' || d[i] == 'E')) {
        i++;
        if (i < n && (d[i] == '+' || d[i] == '-'))
            i++;
        while (i < n && isdigit((unsigned char)d[i]))
            i++;
    }
    return i == n;
}

static result_t object_to_json_recursive_local(pool_t* pool, data_t** dst, const object_t* obj) {
    if (!obj)
        return data_append_str(pool, dst, "null");
    if (obj->data && !obj->child) {
        if (is_json_primitive_local(obj->data)) {
            return data_append_data(pool, dst, obj->data);
        } else {
            data_t* esc = NULL;
            if (escape_json_data(pool, obj->data, &esc) != RESULT_OK)
                RETURN_ERR("Failed to escape string for JSON output");
            if (data_append_char(pool, dst, '"') != RESULT_OK)
                RETURN_ERR("Failed to append to JSON output");
            if (data_append_data(pool, dst, esc) != RESULT_OK)
                RETURN_ERR("Failed to append to JSON output");
            if (data_destroy(pool, esc) != RESULT_OK)
                RETURN_ERR("Failed to free escaped buffer");
            if (data_append_char(pool, dst, '"') != RESULT_OK)
                RETURN_ERR("Failed to append to JSON output");
            return RESULT_OK;
        }
    }
    if (!obj->data && obj->child && obj->child->data && obj->child->child) {
        if (data_append_char(pool, dst, '{') != RESULT_OK)
            RETURN_ERR("Failed to append '{' to JSON output");
        const object_t* ch = obj->child;
        int32_t first = 1;
        while (ch) {
            if (!first) {
                if (data_append_char(pool, dst, ',') != RESULT_OK)
                    RETURN_ERR("Failed to append ',' to JSON output");
            }
            first = 0;
            data_t* esc_key = NULL;
            if (escape_json_data(pool, ch->data, &esc_key) != RESULT_OK)
                RETURN_ERR("Failed to escape object key for JSON output");
            if (data_append_char(pool, dst, '"') != RESULT_OK)
                RETURN_ERR("Failed to append to JSON output");
            if (data_append_data(pool, dst, esc_key) != RESULT_OK)
                RETURN_ERR("Failed to append to JSON output");
            if (data_destroy(pool, esc_key) != RESULT_OK)
                RETURN_ERR("Failed to free escaped key buffer");
            if (data_append_str(pool, dst, "\":") != RESULT_OK)
                RETURN_ERR("Failed to append ':' to JSON output");
            if (object_to_json_recursive_local(pool, dst, ch->child) != RESULT_OK)
                RETURN_ERR("Failed to serialize JSON value");
            ch = ch->next;
        }
        if (data_append_char(pool, dst, '}') != RESULT_OK)
            RETURN_ERR("Failed to append '}' to JSON output");
        return RESULT_OK;
    } else if (!obj->data && obj->child) {
        if (data_append_char(pool, dst, '[') != RESULT_OK)
            RETURN_ERR("Failed to append '[' to JSON output");
        const object_t* ch = obj->child;
        int32_t first = 1;
        while (ch) {
            if (!first) {
                if (data_append_char(pool, dst, ',') != RESULT_OK)
                    RETURN_ERR("Failed to append ',' to JSON output");
            }
            first = 0;
            if (object_to_json_recursive_local(pool, dst, ch) != RESULT_OK)
                RETURN_ERR("Failed to serialize JSON array element");
            ch = ch->next;
        }
        if (data_append_char(pool, dst, ']') != RESULT_OK)
            RETURN_ERR("Failed to append ']' to JSON output");
        return RESULT_OK;
    }
    return data_append_str(pool, dst, "null");
}

result_t object_todata_json(pool_t* pool, data_t** dst, const object_t* src) {
    if (!*dst) {
        if (data_create(pool, dst) != RESULT_OK)
            RETURN_ERR("Failed to create destination data buffer");
    } else {
        if (data_clean(pool, dst) != RESULT_OK)
            RETURN_ERR("Failed to clear destination data buffer");
    }
    return object_to_json_recursive_local(pool, dst, src);
}

result_t object_provide_str(object_t** dst, const object_t* object, const char* path) {
    if (!object || !path) {
        RETURN_ERR("Invalid arguments: object and path are required");
    }
    const object_t* cur = object;
    if (cur->data == NULL && cur->child)
        cur = cur;
    const char* p = path;
    while (*p) {
        char seg[256];
        size_t si = 0;
        while (*p && *p != '.' && si + 1 < sizeof(seg))
            seg[si++] = *p++;
        seg[si] = '\0';
        if (*p == '.')
            p++;
        int32_t is_index = 1;
        for (size_t i = 0; i < si; i++) {
            if (!isdigit((unsigned char)seg[i])) {
                is_index = 0;
                break;
            }
        }
        if (is_index && si > 0) {
            size_t idx = (size_t)strtoull(seg, NULL, 10);
            const object_t* child = cur->child;
            size_t k = 0;
            while (child && k < idx) {
                child = child->next;
                k++;
            }
            if (!child) {
                RETURN_ERR("Array index out of range in path traversal");
            }
            cur = child;
        } else {
            const object_t* child = cur->child;
            int32_t found = 0;
            while (child) {
                if (child->data && child->child) {
                    if (child->data->size == si && memcmp(child->data->data, seg, si) == 0) {
                        cur = child->child;
                        found = 1;
                        break;
                    }
                }
                child = child->next;
            }
            if (!found) {
                RETURN_ERR("Key not found in object during path traversal");
            }
        }
    }
    *dst = (object_t*)cur;
    return RESULT_OK;
}

static const char* skip_xml_ws_local(const char* p, const char* end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        p++;
    return p;
}

static result_t parse_xml_tag_name_local(pool_t* pool, const char** xml, const char* end, data_t** name) {
    const char* p = *xml;
    if (p >= end || (!isalpha((unsigned char)*p) && *p != '_')) {
        RETURN_ERR("Invalid XML tag start: expected letter or '_' ");
    }
    const char* start = p;
    while (p < end && (isalnum((unsigned char)*p) || *p == '-' || *p == '_' || *p == '.' || *p == ':'))
        p++;
    size_t len = (size_t)(p - start);
    if (pool_data_alloc(pool, name, len) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer for XML tag name");
    }
    (*name)->size = len;
    memcpy((*name)->data, start, len);
    *xml = p;
    return RESULT_OK;
}

static result_t parse_xml_text_local(pool_t* pool, const char** xml, const char* end, data_t** out) {
    const char* p = *xml;
    const char* start = p;
    while (p < end && *p != '<')
        p++;
    size_t len = (size_t)(p - start);
    while (len > 0 && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
        len--;
    }
    while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t' || start[len - 1] == '\n' || start[len - 1] == '\r')) {
        len--;
    }
    if (len == 0) {
        *out = NULL;
        *xml = p;
        return RESULT_OK;
    }
    if (pool_data_alloc(pool, out, len) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer for XML text node");
    }
    (*out)->size = len;
    memcpy((*out)->data, start, len);
    *xml = p;
    return RESULT_OK;
}

static result_t parse_xml_element_local(pool_t* pool, const char** xml, const char* end, object_t** out);

static result_t parse_xml_content_local(pool_t* pool, const char** xml, const char* end, const data_t* tag_name, object_t** content) {
    const char* p = *xml;
    p = skip_xml_ws_local(p, end);
    if (p < end && *p == '/') {
        p++;
        p = skip_xml_ws_local(p, end);
        if (p >= end || *p != '>') {
            RETURN_ERR("Malformed self-closing tag: expected '>' after '/'");
        }
        *xml = p + 1;
        return RESULT_OK;
    }
    if (p >= end || *p != '>') {
        RETURN_ERR("Malformed start tag: expected '>' after tag name");
    }
    p++;
    object_t* first = NULL;
    object_t* last = NULL;
    data_t* text_acc = NULL;
    while (p < end) {
        p = skip_xml_ws_local(p, end);
        if (p >= end) {
            RETURN_ERR("Unexpected end of XML while parsing content");
        }
        if (*p == '<') {
            p++;
            if (p < end && *p == '/') {
                p++;
                p = skip_xml_ws_local(p, end);
                data_t* closing = NULL;
                if (parse_xml_tag_name_local(pool, &p, end, &closing) != RESULT_OK) {
                    RETURN_ERR("Failed to parse closing tag name");
                }
                if (closing->size != tag_name->size || memcmp(closing->data, tag_name->data, tag_name->size) != 0) {
                    if (pool_data_free(pool, closing) != RESULT_OK) {
                        RETURN_ERR("Failed to free closing tag buffer");
                    }
                    RETURN_ERR("Mismatched closing tag");
                }
                if (pool_data_free(pool, closing) != RESULT_OK) {
                    RETURN_ERR("Failed to free closing tag buffer");
                }
                p = skip_xml_ws_local(p, end);
                if (p >= end || *p != '>') {
                    RETURN_ERR("Malformed closing tag: expected '>'");
                }
                p++;
                break;
            } else {
                p--;
                object_t* child;
                if (parse_xml_element_local(pool, &p, end, &child) != RESULT_OK) {
                    RETURN_ERR("Failed to parse child XML element");
                }
                if (!first) {
                    first = last = child;
                } else {
                    last->next = child;
                    last = child;
                }
            }
        } else {
            data_t* text = NULL;
            if (parse_xml_text_local(pool, &p, end, &text) != RESULT_OK) {
                RETURN_ERR("Failed to parse XML text node");
            }
            if (text && text->size > 0) {
                if (!text_acc) {
                    text_acc = text;
                } else {
                    if (data_append_data(pool, &text_acc, text) != RESULT_OK) {
                        if (pool_data_free(pool, text) != RESULT_OK) {
                            RETURN_ERR("Failed to free temporary text buffer");
                        }
                        RETURN_ERR("Failed to append text chunk to accumulator");
                    }
                    if (pool_data_free(pool, text) != RESULT_OK) {
                        RETURN_ERR("Failed to free temporary text buffer");
                    }
                }
            } else if (text) {
                if (pool_data_free(pool, text) != RESULT_OK) {
                    RETURN_ERR("Failed to free temporary text buffer");
                }
            }
        }
    }
    if (text_acc && first) {
        RETURN_ERR("Mixed XML content (text + elements) is not supported");
    }
    if (text_acc) {
        (*content)->data = text_acc;
    } else if (first) {
        (*content)->child = first;
    }
    *xml = p;
    return RESULT_OK;
}

static result_t parse_xml_element_local(pool_t* pool, const char** xml, const char* end, object_t** out) {
    const char* p = *xml;
    p = skip_xml_ws_local(p, end);
    if (p >= end || *p != '<') {
        RETURN_ERR("Expected '<' at start of XML element");
    }
    p++;
    data_t* tag = NULL;
    if (parse_xml_tag_name_local(pool, &p, end, &tag) != RESULT_OK) {
        RETURN_ERR("Failed to parse XML tag name");
    }
    object_t* content;
    if (pool_object_alloc(pool, &content) != RESULT_OK) {
        if (pool_data_free(pool, tag) != RESULT_OK)
            RETURN_ERR("Failed to free tag name buffer after alloc failure");
        RETURN_ERR("Failed to allocate object for XML element content");
    }
    content->data = NULL;
    content->child = NULL;
    content->next = NULL;
    if (parse_xml_content_local(pool, &p, end, tag, &content) != RESULT_OK) {
        if (pool_data_free(pool, tag) != RESULT_OK)
            RETURN_ERR("Failed to free tag name buffer after content parse failure");
        RETURN_ERR("Failed to parse XML element content");
    }
    if (pool_object_alloc(pool, out) != RESULT_OK) {
        if (pool_data_free(pool, tag) != RESULT_OK)
            RETURN_ERR("Failed to free tag name buffer after pair alloc failure");
        RETURN_ERR("Failed to allocate object for XML key/value pair");
    }
    (*out)->data = tag;
    (*out)->child = content;
    (*out)->next = NULL;
    *xml = p;
    return RESULT_OK;
}

result_t object_parse_xml(pool_t* pool, object_t** dst, const data_t* src) {
    if (!pool || !dst || !src) {
        RETURN_ERR("Invalid arguments: pool, dst, and src are required for XML parsing");
    }
    if (src->size == 0) {
        RETURN_ERR("Empty XML input");
    }
    const char* xml = src->data;
    const char* end = src->data + src->size;
    const char* p = skip_xml_ws_local(xml, end);
    if (p < end && *p == '<' && p + 1 < end && p[1] == '?') {
        while (p < end && !(p[0] == '?' && p + 1 < end && p[1] == '>'))
            p++;
        if (p < end)
            p += 2;
        p = skip_xml_ws_local(p, end);
    }
    if (pool_object_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate root XML object");
    }
    (*dst)->data = NULL;
    (*dst)->child = NULL;
    (*dst)->next = NULL;
    object_t* first = NULL;
    object_t* last = NULL;
    while (p < end) {
        p = skip_xml_ws_local(p, end);
        if (p >= end)
            break;
        if (*p == '<') {
            if (p + 4 <= end && strncmp(p, "<!--", 4) == 0) {
                p += 4;
                while (p + 3 <= end && strncmp(p, "-->", 3) != 0)
                    p++;
                if (p + 3 <= end)
                    p += 3;
                continue;
            }
            if (p + 2 <= end && p[1] == '!') {
                while (p < end && *p != '>')
                    p++;
                if (p < end)
                    p++;
                continue;
            }
            object_t* elem;
            if (parse_xml_element_local(pool, &p, end, &elem) != RESULT_OK) {
                RETURN_ERR("Failed to parse XML element");
            }
            if (!first) {
                first = last = elem;
            } else {
                last->next = elem;
                last = elem;
            }
        } else {
            while (p < end && *p != '<')
                p++;
        }
    }
    (*dst)->child = first;
    return RESULT_OK;
}

static result_t escape_xml_data(pool_t* pool, const data_t* in, data_t** out) {
    size_t est = in ? in->size * 6 + 1 : 1;
    if (pool_data_alloc(pool, out, est) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer for XML-escaped data");
    }
    (*out)->size = 0;
    if (!in)
        return RESULT_OK;
    for (uint64_t i = 0; i < in->size; i++) {
        char ch = in->data[i];
        switch (ch) {
            case '<':
                if (data_append_str(pool, out, "&lt;") != RESULT_OK) {
                    RETURN_ERR("Failed to append '&lt;' during XML escape");
                }
                break;
            case '>':
                if (data_append_str(pool, out, "&gt;") != RESULT_OK) {
                    RETURN_ERR("Failed to append '&gt;' during XML escape");
                }
                break;
            case '&':
                if (data_append_str(pool, out, "&amp;") != RESULT_OK) {
                    RETURN_ERR("Failed to append '&amp;' during XML escape");
                }
                break;
            case '"':
                if (data_append_str(pool, out, "&quot;") != RESULT_OK) {
                    RETURN_ERR("Failed to append '&quot;' during XML escape");
                }
                break;
            case '\'':
                if (data_append_str(pool, out, "&apos;") != RESULT_OK) {
                    RETURN_ERR("Failed to append '&apos;' during XML escape");
                }
                break;
            default:
                if ((unsigned char)ch < 0x20 && ch != '\t' && ch != '\n' && ch != '\r') {
                } else {
                    if (data_append_char(pool, out, ch) != RESULT_OK) {
                        RETURN_ERR("Failed to append raw character during XML escape");
                    }
                }
        }
    }
    return RESULT_OK;
}

static int32_t data_lexcmp_local(const data_t* a, const data_t* b) {
    if (!a && !b)
        return 0;
    if (!a)
        return -1;
    if (!b)
        return 1;
    uint64_t min = a->size < b->size ? a->size : b->size;
    int32_t cmp = memcmp(a->data, b->data, min);
    if (cmp != 0)
        return cmp;
    if (a->size < b->size)
        return -1;
    if (a->size > b->size)
        return 1;
    return 0;
}

static result_t object_to_xml_recursive_local(pool_t* pool, data_t** dst, const object_t* src, const char* element_name) {
    if (!src) {
        if (data_append_str(pool, dst, "<") != RESULT_OK) {
            RETURN_ERR("Failed to append '<' while serializing empty element");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing empty element");
        }
        if (data_append_str(pool, dst, "/>") != RESULT_OK) {
            RETURN_ERR("Failed to append '/>' while serializing empty element");
        }
        return RESULT_OK;
    }
    if (src->data && !src->child) {
        data_t* esc = NULL;
        if (escape_xml_data(pool, src->data, &esc) != RESULT_OK) {
            RETURN_ERR("Failed to escape XML text content");
        }
        if (data_append_str(pool, dst, "<") != RESULT_OK) {
            RETURN_ERR("Failed to append '<' while serializing element start");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing element start");
        }
        if (data_append_str(pool, dst, ">") != RESULT_OK) {
            RETURN_ERR("Failed to append '>' while serializing element start");
        }
        if (data_append_data(pool, dst, esc) != RESULT_OK) {
            RETURN_ERR("Failed to append escaped text content to XML output");
        }
        if (data_append_str(pool, dst, "</") != RESULT_OK) {
            RETURN_ERR("Failed to append '</' while serializing element end");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing element end");
        }
        if (data_append_str(pool, dst, ">") != RESULT_OK) {
            RETURN_ERR("Failed to append '>' while serializing element end");
        }
        if (pool_data_free(pool, esc) != RESULT_OK) {
            RETURN_ERR("Failed to free temporary XML-escaped buffer");
        }
        return RESULT_OK;
    }
    object_t* first = src->child;
    if (first && first->data) {
        if (data_append_str(pool, dst, "<") != RESULT_OK) {
            RETURN_ERR("Failed to append '<' while serializing object start");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing object start");
        }
        if (data_append_str(pool, dst, ">") != RESULT_OK) {
            RETURN_ERR("Failed to append '>' while serializing object start");
        }
        const data_t* prev_key = NULL;
        const object_t* prev_child = NULL;
        size_t emitted = 0;
        size_t count = 0;
        {
            for (object_t* c = first; c; c = c->next)
                if (c->data)
                    count++;
        }
        while (emitted < count) {
            object_t* best = NULL;
            for (object_t* c = first; c; c = c->next) {
                if (!c->data)
                    continue;
                if (!prev_key) {
                    int32_t rel = data_lexcmp_local(c->data, best ? best->data : NULL);
                    if (!best || rel < 0 || (rel == 0 && c < best))
                        best = c;
                } else {
                    int32_t cmp_prev = data_lexcmp_local(c->data, prev_key);
                    if (cmp_prev < 0)
                        continue;
                    if (cmp_prev == 0 && prev_child && c <= prev_child)
                        continue;
                    int32_t rel = data_lexcmp_local(c->data, best ? best->data : NULL);
                    if (!best || rel < 0 || (rel == 0 && c < best))
                        best = c;
                }
            }
            if (!best)
                break;
            data_t* key_esc = NULL;
            if (escape_xml_data(pool, best->data, &key_esc) != RESULT_OK)
                RETURN_ERR("Failed to XML-escape object key");
            data_t* key_cstr = NULL;
            if (pool_data_alloc(pool, &key_cstr, key_esc->size + 1) != RESULT_OK) {
                if (pool_data_free(pool, key_esc) != RESULT_OK)
                    RETURN_ERR("Failed to free escaped key buffer after alloc failure");
                RETURN_ERR("Failed to allocate C-string buffer for XML key");
            }
            memcpy(key_cstr->data, key_esc->data, key_esc->size);
            key_cstr->data[key_esc->size] = '\0';
            key_cstr->size = key_esc->size;
            if (object_to_xml_recursive_local(pool, dst, best->child, key_cstr->data) != RESULT_OK) {
                if (pool_data_free(pool, key_cstr) != RESULT_OK)
                    RETURN_ERR("Failed to free key cstring buffer after child serialization failure");
                if (pool_data_free(pool, key_esc) != RESULT_OK)
                    RETURN_ERR("Failed to free escaped key buffer after child serialization failure");
                RETURN_ERR("Failed to serialize child element for object key");
            }
            if (pool_data_free(pool, key_cstr) != RESULT_OK)
                RETURN_ERR("Failed to free key cstring buffer");
            if (pool_data_free(pool, key_esc) != RESULT_OK)
                RETURN_ERR("Failed to free escaped key buffer");
            prev_key = best->data;
            prev_child = best;
            emitted++;
        }
        if (data_append_str(pool, dst, "</") != RESULT_OK) {
            RETURN_ERR("Failed to append '</' while serializing object end");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing object end");
        }
        if (data_append_str(pool, dst, ">") != RESULT_OK) {
            RETURN_ERR("Failed to append '>' while serializing object end");
        }
        return RESULT_OK;
    } else if (first) {
        if (data_append_str(pool, dst, "<") != RESULT_OK) {
            RETURN_ERR("Failed to append '<' while serializing array start");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing array start");
        }
        if (data_append_str(pool, dst, ">") != RESULT_OK) {
            RETURN_ERR("Failed to append '>' while serializing array start");
        }
        int32_t index = 0;
        for (object_t* c = first; c; c = c->next, index++) {
            char item_name[32];
            snprintf(item_name, sizeof(item_name), "item%d", index);
            if (object_to_xml_recursive_local(pool, dst, c, item_name) != RESULT_OK)
                RETURN_ERR("Failed to serialize array item element");
        }
        if (data_append_str(pool, dst, "</") != RESULT_OK) {
            RETURN_ERR("Failed to append '</' while serializing array end");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing array end");
        }
        if (data_append_str(pool, dst, ">") != RESULT_OK) {
            RETURN_ERR("Failed to append '>' while serializing array end");
        }
        return RESULT_OK;
    } else {
        if (data_append_str(pool, dst, "<") != RESULT_OK) {
            RETURN_ERR("Failed to append '<' while serializing empty element");
        }
        if (data_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name while serializing empty element");
        }
        if (data_append_str(pool, dst, "/>") != RESULT_OK) {
            RETURN_ERR("Failed to append '/>' while serializing empty element");
        }
        return RESULT_OK;
    }
}

result_t object_todata_xml(pool_t* pool, data_t** dst, const object_t* src) {
    if (!*dst) {
        if (data_create(pool, dst) != RESULT_OK)
            RETURN_ERR("Failed to create destination buffer for XML output");
    } else {
        if (data_clean(pool, dst) != RESULT_OK)
            RETURN_ERR("Failed to clear destination buffer for XML output");
    }
    if (src && src->child) {
        object_t* first = src->child;
        if (first && first->data) {
            const data_t* prev_key = NULL;
            const object_t* prev_child = NULL;
            size_t emitted = 0;
            size_t count = 0;
            {
                for (object_t* c = first; c; c = c->next)
                    if (c->data)
                        count++;
            }
            while (emitted < count) {
                object_t* best = NULL;
                for (object_t* c = first; c; c = c->next) {
                    if (!c->data)
                        continue;
                    if (!prev_key) {
                        int32_t rel = data_lexcmp_local(c->data, best ? best->data : NULL);
                        if (!best || rel < 0 || (rel == 0 && c < best))
                            best = c;
                    } else {
                        int32_t cmp_prev = data_lexcmp_local(c->data, prev_key);
                        if (cmp_prev < 0)
                            continue;
                        if (cmp_prev == 0 && prev_child && c <= prev_child)
                            continue;
                        int32_t rel = data_lexcmp_local(c->data, best ? best->data : NULL);
                        if (!best || rel < 0 || (rel == 0 && c < best))
                            best = c;
                    }
                }
                if (!best)
                    break;
                data_t* key_esc = NULL;
                if (escape_xml_data(pool, best->data, &key_esc) != RESULT_OK)
                    RETURN_ERR("Failed to XML-escape object key");
                data_t* key_cstr = NULL;
                if (pool_data_alloc(pool, &key_cstr, key_esc->size + 1) != RESULT_OK) {
                    if (pool_data_free(pool, key_esc) != RESULT_OK)
                        RETURN_ERR("Failed to free escaped key buffer after alloc failure");
                    RETURN_ERR("Failed to allocate C-string buffer for XML key");
                }
                memcpy(key_cstr->data, key_esc->data, key_esc->size);
                key_cstr->data[key_esc->size] = '\0';
                key_cstr->size = key_esc->size;
                if (object_to_xml_recursive_local(pool, dst, best->child, key_cstr->data) != RESULT_OK) {
                    if (pool_data_free(pool, key_cstr) != RESULT_OK)
                        RETURN_ERR("Failed to free key cstring buffer after child serialization failure");
                    if (pool_data_free(pool, key_esc) != RESULT_OK)
                        RETURN_ERR("Failed to free escaped key buffer after child serialization failure");
                    RETURN_ERR("Failed to serialize child element for object key");
                }
                if (pool_data_free(pool, key_cstr) != RESULT_OK)
                    RETURN_ERR("Failed to free key cstring buffer");
                if (pool_data_free(pool, key_esc) != RESULT_OK)
                    RETURN_ERR("Failed to free escaped key buffer");
                prev_key = best->data;
                prev_child = best;
                emitted++;
            }
            return RESULT_OK;
        } else {
            int32_t index = 0;
            for (object_t* c = first; c; c = c->next, index++) {
                char item_name[32];
                snprintf(item_name, sizeof(item_name), "item%d", index);
                if (object_to_xml_recursive_local(pool, dst, c, item_name) != RESULT_OK)
                    RETURN_ERR("Failed to serialize array item element");
            }
            return RESULT_OK;
        }
    } else {
        return object_to_xml_recursive_local(pool, dst, src, "value");
    }
}
