#include "lkjscript.h"

static int64_t utf8_char_width(const char* s) {
    if (!s || !*s) {
        return 0;
    }

    unsigned char c = (unsigned char)*s;

    if (c < 0x80) {
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        if (!*(s + 1) || (*(unsigned char*)(s + 1) & 0xC0) != 0x80)
            return 1;
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        if (!*(s + 1) || (*(unsigned char*)(s + 1) & 0xC0) != 0x80 ||
            !*(s + 2) || (*(unsigned char*)(s + 2) & 0xC0) != 0x80)
            return 1;
        return 3;
    } else if ((c & 0xF8) == 0xF0) {
        if (!*(s + 1) || (*(unsigned char*)(s + 1) & 0xC0) != 0x80 ||
            !*(s + 2) || (*(unsigned char*)(s + 2) & 0xC0) != 0x80 ||
            !*(s + 3) || (*(unsigned char*)(s + 3) & 0xC0) != 0x80)
            return 1;
        return 4;
    } else {
        return 1;
    }
}

static const char* my_strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return str;
        }
        str++;
    }
    if ((char)c == '\0') {
        return str;
    }
    return NULL;
}

result_t tokenize(const char* src, token_t* token) {
    const char* src_base = src;
    const char* src_itr = src;
    token_t* token_itr = token;

    int is_multiline_comment = 0;
    int is_string = 0;

    while (*src_itr) {
        if (is_multiline_comment) {
            if (*src_itr == '*' && *(src_itr + 1) == '/') {
                is_multiline_comment = 0;
                src_itr += 2;
                src_base = src_itr;
            } else {
                src_itr += 1;
            }
        } else if (is_string) {
            if (*src_itr == '\\') {
                if (*(src_itr + 1)) {
                    src_itr += 2;
                } else {
                    src_itr += 1;
                }
            } else if (*src_itr == '"') {
                src_itr += 1;
                *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
                is_string = 0;
                src_base = src_itr;
            } else {
                int char_len = utf8_char_width(src_itr);
                src_itr += (char_len > 0 ? char_len : 1);
            }
        } else if (*src_itr == '/' && *(src_itr + 1) == '*') {
            if (src_base != src_itr) {
                *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
            }
            is_multiline_comment = 1;
            src_itr += 2;
            src_base = src_itr;
        } else if (*src_itr == '/' && *(src_itr + 1) == '/') {
            if (src_base != src_itr) {
                *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
            }
            while (*src_itr && *src_itr != '\n') {
                src_itr++;
            }
            src_base = src_itr;
        } else if (*src_itr == '"') {
            if (src_base != src_itr) {
                *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
            }
            is_string = 1;
            src_base = src_itr;
            src_itr += 1;
        } else if (*src_itr == ' ' || *src_itr == '\t' || *src_itr == '\r') {
            if (src_base != src_itr) {
                *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
            }
            src_itr += 1;
            src_base = src_itr;
        } else {
            char ch1 = *src_itr;
            char ch2 = (*(src_itr + 1) != '\0') ? *(src_itr + 1) : '\0';

            int op_found = 0;

            if (ch2 != '\0') {
                if ((ch1 == '<' && ch2 == '<') || (ch1 == '>' && ch2 == '>') ||
                    (ch1 == '<' && ch2 == '=') || (ch1 == '>' && ch2 == '=') ||
                    (ch1 == '=' && ch2 == '=') || (ch1 == '!' && ch2 == '=') ||
                    (ch1 == '&' && ch2 == '&') || (ch1 == '|' && ch2 == '|')) {
                    if (src_base != src_itr) {
                        *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
                    }
                    *(token_itr++) = (token_t){.data = src_itr, .size = 2};
                    src_itr += 2;
                    src_base = src_itr;
                    op_found = 1;
                }
            }

            if (op_found) {
                continue;
            }

            const char* single_char_ops_str = "(){};,:.*%&|^~<>!=+-/\n";
            if (my_strchr(single_char_ops_str, ch1)) {
                if (src_base != src_itr) {
                    *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
                }
                *(token_itr++) = (token_t){.data = src_itr, .size = 1};
                src_itr += 1;
                src_base = src_itr;
                op_found = 1;
            }

            if (op_found) {
                continue;
            }

            int char_len = utf8_char_width(src_itr);
            src_itr += (char_len > 0 ? char_len : 1);
        }
    }

    if (src_base != src_itr) {
        *(token_itr++) = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
    }

    *token_itr = (token_t){.data = NULL, .size = 0};

    return OK;
}