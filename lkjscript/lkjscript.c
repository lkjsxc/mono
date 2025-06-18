#include "lkjscript.h"
#include "exec.c"
#include "genbin.c"
#include "parse.c"
#include "readsrc.c"
#include "token.c"
#include "tokenize.c"

static mem_t mem;

__attribute__((warn_unused_result))
result_t compile() {
    if (readsrc(SRC_PATH, mem.compile.src, sizeof(mem.compile.src)) == ERR) {
        write(STDERR_FILENO, "Error reading source file\n", 26);
        return ERR;
    }
    if (tokenize(mem.compile.src, mem.compile.token) == ERR) {
        write(STDERR_FILENO, "Error tokenizing source\n", 24);
        return ERR;
    }
    if (parse(mem.compile.token, mem.compile.node) == ERR) {
        write(STDERR_FILENO, "Error parsing tokens\n", 21);
        return ERR;
    }
    if (genbin(mem.compile.node, mem.compile.bin) == ERR) {
        write(STDERR_FILENO, "Error generating binary\n", 24);
        return ERR;
    }
    return OK;
}

int main() {
    if (compile() == ERR) {
        write(STDERR_FILENO, "Compilation failed\n", 19);
        return 1;
    }
    if (exec(mem.u8) == ERR) {
        write(STDERR_FILENO, "Execution failed\n", 19);
        return 1;
    }
    return 0;
}