#include "lkjscript.h"

static int64_t utf8_char_width(const char* s) {
    if (!s || !*s) {
        return 0;
    }

    unsigned char c = (unsigned char)*s;

    if (c < 0x80) {
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        if (c < 0xC2 ||
            !*(s + 1) || (*(unsigned char*)(s + 1) & 0xC0) != 0x80) {
            return 1;
        }
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        if (!*(s + 1) || (*(unsigned char*)(s + 1) & 0xC0) != 0x80 ||
            !*(s + 2) || (*(unsigned char*)(s + 2) & 0xC0) != 0x80) {
            return 1;
        }
        if (c == 0xE0 && (unsigned char)*(s + 1) < 0xA0) {
            return 1;
        }
        if (c == 0xED && (unsigned char)*(s + 1) >= 0xA0) {
            return 1;
        }
        return 3;
    } else if ((c & 0xF8) == 0xF0) {
        if (!*(s + 1) || (*(unsigned char*)(s + 1) & 0xC0) != 0x80 ||
            !*(s + 2) || (*(unsigned char*)(s + 2) & 0xC0) != 0x80 ||
            !*(s + 3) || (*(unsigned char*)(s + 3) & 0xC0) != 0x80) {
            return 1;
        }
        if (c == 0xF0 && (unsigned char)*(s + 1) < 0x90) {
            return 1;
        }
        if (c > 0xF4 || (c == 0xF4 && (unsigned char)*(s + 1) > 0x8F)) {
            return 1;
        }
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

static void emit_pending_token(const char** p_src_base, const char* src_itr, token_t** p_token_itr) {
    if (src_itr > *p_src_base) {
        *(*p_token_itr)++ = (token_t){.data = *p_src_base, .size = (int64_t)(src_itr - *p_src_base)};
    }
}

__attribute__((warn_unused_result))
result_t tokenize(const char* src, token_t* token_array) {
    const char* src_base = src;
    const char* src_itr = src;
    token_t* token_itr = token_array;

    int is_multiline_comment = 0;
    int is_string = 0;

    while (*src_itr) {
        if (!is_string && !is_multiline_comment && *src_itr == '\\') {
            int consumed_len = 0;
            if (*(src_itr + 1) == '\n') {
                consumed_len = 2;
            }
            else if (*(src_itr + 1) == '\r' && *(src_itr + 2) == '\n') {
                consumed_len = 3;
            }

            if (consumed_len > 0) {
                emit_pending_token(&src_base, src_itr, &token_itr);
                src_itr += consumed_len;
                src_base = src_itr;
                continue;
            }
        }

        if (is_multiline_comment) {
            if (*src_itr == '*' && *(src_itr + 1) == '/') {
                is_multiline_comment = 0;
                src_itr += 2;
                src_base = src_itr;
            } else {
                src_itr++;
            }
            continue;
        }

        if (is_string) {
            if (*src_itr == '\\') {
                if (*(src_itr + 1)) {
                    src_itr += 2;
                } else {
                    src_itr += 1;
                }
            } else if (*src_itr == '"') {
                src_itr++;
                *token_itr++ = (token_t){.data = src_base, .size = (int64_t)(src_itr - src_base)};
                is_string = 0;
                src_base = src_itr;
            } else {
                int64_t char_len = utf8_char_width(src_itr);
                src_itr += (char_len > 0 ? char_len : 1);
            }
            continue;
        }

        if (*src_itr == '/' && *(src_itr + 1) == '*') {
            emit_pending_token(&src_base, src_itr, &token_itr);
            is_multiline_comment = 1;
            src_itr += 2;
            src_base = src_itr;
            continue;
        }

        if (*src_itr == '/' && *(src_itr + 1) == '/') {
            emit_pending_token(&src_base, src_itr, &token_itr);
            while (*src_itr && *src_itr != '\n') {
                src_itr++;
            }
            src_base = src_itr;
            continue;
        }

        if (*src_itr == '"') {
            emit_pending_token(&src_base, src_itr, &token_itr);
            is_string = 1;
            src_base = src_itr;
            src_itr++;
            continue;
        }

        if (*src_itr == ' ' || *src_itr == '\t' || *src_itr == '\r') {
            emit_pending_token(&src_base, src_itr, &token_itr);
            src_itr++;
            src_base = src_itr;
            continue;
        }

        char ch1 = *src_itr;
        char ch2 = (*(src_itr + 1) != '\0') ? *(src_itr + 1) : '\0';

        int op_len = 0;
        if (ch2 != '\0') {
            if ((ch1 == '<' && ch2 == '<') || (ch1 == '>' && ch2 == '>') ||
                (ch1 == '<' && ch2 == '=') || (ch1 == '>' && ch2 == '=') ||
                (ch1 == '=' && ch2 == '=') || (ch1 == '!' && ch2 == '=') ||
                (ch1 == '&' && ch2 == '&') || (ch1 == '|' && ch2 == '|') ||
                (ch1 == '-' && ch2 == '>')) {
                op_len = 2;
            }
        }

        if (op_len > 0) {
            emit_pending_token(&src_base, src_itr, &token_itr);
            *token_itr++ = (token_t){.data = src_itr, .size = op_len};
            src_itr += op_len;
            src_base = src_itr;
            continue;
        }

        const char* single_char_ops_str = "(){};,:.*%&|^~<>!=+-/\n";
        if (my_strchr(single_char_ops_str, ch1)) {
            emit_pending_token(&src_base, src_itr, &token_itr);
            *token_itr++ = (token_t){.data = src_itr, .size = 1};
            src_itr++;
            src_base = src_itr;
            continue;
        }

        int64_t char_w = utf8_char_width(src_itr);
        src_itr += (char_w > 0 ? char_w : 1);
    }

    if (is_multiline_comment) {
        return ERR;
    }
    if (is_string) {
        return ERR;
    }

    emit_pending_token(&src_base, src_itr, &token_itr);

    *token_itr = (token_t){.data = NULL, .size = 0};

    return OK;
}