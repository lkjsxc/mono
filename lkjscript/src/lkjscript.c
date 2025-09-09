#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LKJSCRIPT_SIZE (1024 * 1024 * 16)
#define LKJSCRIPT_SRCPATH "/data/0001.lkjscript"

typedef enum inst_t inst_t;
typedef struct lkjscript_t lkjscript_t;
typedef struct instmap_t instmap_t;

enum inst_t {
    INST_NULL,
    INST_END,
    INST_DEBUG,
    INST_IMM,
    INST_COPY,
};

struct lkjscript_t {
    uint8_t data[LKJSCRIPT_SIZE];
};

struct instmap_t {
    const char* name;
    inst_t inst;
};

static instmap_t instmap[] = {
    {"null", INST_NULL},
    {"end", INST_END},
    {"debug", INST_DEBUG},
    {"imm", INST_IMM},
    {"copy", INST_COPY},
};

static lkjscript_t lkjscript;

int32_t token_size(const char* token) {
    for (int32_t i = 0; 1; i++) {
        if (token[i] == ' ' || token[i] == '\n' || token[i] == '\r' || token[i] == '\t' || token[i] == '\0') {
            return i;
        }
    }
}

int32_t token_toint(const char* token) {
    int32_t size = token_size(token);
    int32_t value = 0;
    for (int32_t i = 0; i < size; i++) {
        value = value * 10 + (token[i] - '0');
    }
    return value;
}

int32_t token_equal(const char* a, const char* b) {
    int32_t a_size = token_size(a);
    int32_t b_size = token_size(b);
    if (a_size != b_size) {
        return 0;
    }
    for (int32_t i = 0; i < a_size; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

const char* token_skipspace(const char* token) {
    while (*token == ' ' || *token == '\n' || *token == '\r' || *token == '\t') {
        token++;
    }
    return token;
}

const char* token_next(const char* token) {
    int32_t size = token_size(token);
    token += size;
    token = token_skipspace(token);
    while (token_equal(token, "//")) {
        while (*token != '\n' && *token != '\0') {
            token++;
        }
        token = token_skipspace(token);
    }
    return token;
}

void compile() {
    // Load source file
    FILE* src_file = fopen(LKJSCRIPT_SRCPATH, "rb");
    if (!src_file) {
        fprintf(stderr, "Error: Unable to open source file %s\n", LKJSCRIPT_SRCPATH);
        exit(1);
    }
    int32_t src_size = fread(lkjscript.data + 1, 1, LKJSCRIPT_SIZE, src_file);
    fclose(src_file);
    lkjscript.data[0] = ' ';
    lkjscript.data[src_size + 1] = '\0';

    // Tokenize and compile
    int32_t* inst_data = (int32_t*)(lkjscript.data + src_size + 2);
    int32_t inst_size = 0;
    const char* token_itr = (const char*)lkjscript.data;
    token_itr = token_next(token_itr);
    while (*token_itr != '\0') {
        if (token_equal(token_itr, "debug")) {
            token_itr = token_next(token_itr);
            inst_data[inst_size++] = (INST_DEBUG << 24);
            continue;
        }
        if (token_equal(token_itr, "imm")) {
            token_itr = token_next(token_itr);
            int32_t reg = token_toint(token_itr);
            token_itr = token_next(token_itr);
            int32_t value = token_toint(token_itr);
            token_itr = token_next(token_itr);
            inst_data[inst_size++] = (INST_IMM << 24) | (reg << 16) | value;
            continue;
        }
        if (token_equal(token_itr, "copy")) {
            token_itr = token_next(token_itr);
            int32_t reg_dst = token_toint(token_itr);
            token_itr = token_next(token_itr);
            int32_t reg_src = token_toint(token_itr);
            token_itr = token_next(token_itr);
            inst_data[inst_size++] = (INST_COPY << 24) | (reg_dst << 16) | reg_src;
            continue;
        }
        fprintf(stderr, "Error: Unknown token '%.*s' token[0]: %d\n", token_size(token_itr), token_itr, token_itr[0]);
        exit(1);
    }
    inst_data[inst_size++] = (INST_END << 24);

    // Move instructions to the start of lkjscript.data
    memmove(lkjscript.data, inst_data, inst_size * sizeof(int32_t));
}

void run() {
    int64_t reg[16];
    uint8_t* pc = lkjscript.data;
    while (1) {
        uint32_t inst = *((uint32_t*)pc);
        pc += 4;
        switch (inst >> 24) {
            case INST_END:
                exit(0);
            case INST_DEBUG: {
                printf("Debug: reg[0]=%ld\n", reg[0]);
                fflush(stdout);
            } break;
            case INST_IMM: {
                uint32_t reg_id = (inst >> 16) & 0xFF;
                uint32_t value = inst & 0xFFFF;
                reg[reg_id] = value;
            } break;
            case INST_COPY: {
                uint32_t reg_dst = (inst >> 16) & 0xFF;
                uint32_t reg_src = inst & 0xFFFF;
                reg[reg_dst] = reg[reg_src];
            } break;
        }
    }
}

int main() {
    compile();
    run();
}
