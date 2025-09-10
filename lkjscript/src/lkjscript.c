#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int assemble_arm64(unsigned char* code_buf, const char* assembly_source) {
    char* source_copy = strdup(assembly_source);
    if (!source_copy) {
        perror("strdup");
        return -1;
    }
    int code_index = 0;
    char* line = strtok(source_copy, "\n");
    while (line) {
        while (*line == ' ' || *line == '\t'){
            line++;}
        if (strlen(line) == 0) {
            line = strtok(NULL, "\n");
            continue;
        }
        char mnemonic[16] = {0};
        sscanf(line, "%15s", mnemonic);
        uint32_t instruction = 0;
        long imm = 0;
        if (strcmp(mnemonic, "RET") != 0) {
            char* immediate_part = strrchr(line, '#');
            if (!immediate_part) {
                fprintf(stderr, "Error: Immediate value starting with '#' not found in line: '%s'\n", line);
                free(source_copy);
                return -1;
            }
            char* endptr;
            imm = strtol(immediate_part + 1, &endptr, 0);
            if (endptr == immediate_part + 1) {
                fprintf(stderr, "Error: Invalid immediate value in line: '%s'\n", line);
                free(source_copy);
                return -1;
            }
        }
        if (strcmp(mnemonic, "MOV") == 0) {
            if (imm < 0 || imm > 65535) {
                fprintf(stderr, "Error: Immediate for MOV must be between 0 and 65535. Line: '%s'\n", line);
                free(source_copy);
                return -1;
            }
            instruction = 0xD2800000 | ((uint16_t)imm << 5);
        } else if (strcmp(mnemonic, "ADD") == 0) {
            if (imm < 0 || imm > 4095) {
                fprintf(stderr, "Error: Immediate for ADD must be between 0 and 4095. Line: '%s'\n", line);
                free(source_copy);
                return -1;
            }
            instruction = 0x91000000 | ((uint16_t)imm << 10);
        } else if (strcmp(mnemonic, "SUB") == 0) {
            if (imm < 0 || imm > 4095) {
                fprintf(stderr, "Error: Immediate for SUB must be between 0 and 4095. Line: '%s'\n", line);
                free(source_copy);
                return -1;
            }
            instruction = 0xD1000000 | ((uint16_t)imm << 10);
        } else if (strcmp(mnemonic, "RET") == 0) {
            instruction = 0xD65F03C0;
        } else {
            fprintf(stderr, "Error: Unknown instruction or format: '%s'\n", line);
            free(source_copy);
            return -1;
        }
        memcpy(&code_buf[code_index], &instruction, sizeof(uint32_t));
        code_index += sizeof(uint32_t);
        line = strtok(NULL, "\n");
    }
    free(source_copy);
    return code_index;
}

int execute_code(const unsigned char* machine_code, int code_size) {
    void* mem = mmap(NULL, code_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    memcpy(mem, machine_code, code_size);
    if (mprotect(mem, code_size, PROT_READ | PROT_EXEC) == -1) {
        perror("mprotect");
        munmap(mem, code_size);
        return -1;
    }
    __builtin___clear_cache((char*)mem, (char*)mem + code_size);
    int (*func)() = (int (*)())mem;
    int result = func();
    munmap(mem, code_size);
    return result;
}

int main() {
    const char* simple_assembly =
        "MOV x0, #100\n"
        "ADD x0, x0, #20\n"
        "SUB x0, x0, #3\n"
        "RET\n";
    printf("--- Original Assembly Code (for ARM64) ---\n");
    printf("%s", simple_assembly);
    printf("-----------------------------------------\n\n");
    unsigned char machine_code[4096];
    int code_size = assemble_arm64(machine_code, simple_assembly);
    if (code_size < 0) {
        fprintf(stderr, "Assembly failed.\n");
        return 1;
    }
    printf("--- Assembled Machine Code (ARM64) ---\n");
    for (int i = 0; i < code_size; ++i) {
        printf("%02X ", machine_code[i]);
        if ((i + 1) % 4 == 0)
            printf("\n");
    }
    printf("--------------------------------------\n\n");

    int result = execute_code(machine_code, code_size);
    printf("Expected result: 100 + 20 - 3 = 117\n");
    printf("Execution result (Value of x0): %d\n", result);

    return 0;
}