#include "lkjscript.h"

result_t compile_tokenize(token_t* token_data, const char* src_data) {
    token_t* token_itr = token_data;
    const char* base_itr = src_data;
    const char* corrent_itr = src_data;
    bool_t iscomment = FALSE;
    while (1) {
        char ch1 = *(corrent_itr + 0);
        char ch2 = *(corrent_itr + 1);
        if (ch1 == '\0') {
            break;
        } else if (ch1 == '\n') {
            if (!iscomment && base_itr != corrent_itr) {
                *(token_itr++) = (token_t){.data = base_itr, .size = corrent_itr - base_itr};
            }
            iscomment = FALSE;
            *(token_itr++) = (token_t){.data = corrent_itr, .size = 1};
            corrent_itr += 1;
            base_itr = corrent_itr;
        } else if (ch1 == '/' && ch2 == '/') {
            iscomment = TRUE;
            corrent_itr += 1;
        } else if (iscomment) {
            corrent_itr += 1;
        } else if (ch1 == ' ') {
            if (base_itr != corrent_itr) {
                *(token_itr++) = (token_t){.data = base_itr, .size = corrent_itr - base_itr};
            }
            corrent_itr += 1;
            base_itr = corrent_itr;
        } else if (
            (ch1 == '<' && ch2 == '<') ||
            (ch1 == '>' && ch2 == '>') ||
            (ch1 == '<' && ch2 == '=') ||
            (ch1 == '>' && ch2 == '=') ||
            (ch1 == '=' && ch2 == '=') ||
            (ch1 == '!' && ch2 == '=') ||
            (ch1 == '&' && ch2 == '&') ||
            (ch1 == '|' && ch2 == '|')) {
            if (base_itr != corrent_itr) {
                *(token_itr++) = (token_t){.data = base_itr, .size = corrent_itr - base_itr};
            }
            *(token_itr++) = (token_t){.data = corrent_itr, .size = 2};
            corrent_itr += 2;
            base_itr = corrent_itr;
        } else if (ch1 == '(' || ch1 == ')' || ch1 == '{' || ch1 == '}' || ch1 == ';' || ch1 == ',' ||
                   ch1 == ':' || ch1 == '.' || ch1 == '+' || ch1 == '-' || ch1 == '*' || ch1 == '/' ||
                   ch1 == '%' || ch1 == '&' || ch1 == '|' || ch1 == '^' || ch1 == '~' || ch1 == '<' ||
                   ch1 == '>' || ch1 == '!' || ch1 == '=') {
            if (base_itr != corrent_itr) {
                *(token_itr++) = (token_t){.data = base_itr, .size = corrent_itr - base_itr};
            }
            *(token_itr++) = (token_t){.data = corrent_itr, .size = 1};
            corrent_itr += 1;
            base_itr = corrent_itr;
        } else {
            corrent_itr += 1;
        }
    }
    *(token_itr++) = (token_t){.data = NULL, .size = 0};
    return OK;
}