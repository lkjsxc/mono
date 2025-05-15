#include "lkjscript.h"
#include "readsrc.c"
#include "token.c"
#include "parse.c"

static mem_t mem;

result_t compile() {
    if (readsrc(SRC_PATH, mem.compile.src, sizeof(mem.compile.src)) == ERR) {
        write(STDERR_FILENO, "Error reading source file\n", 26);
        return ERR;
    }
    if (tokenize(mem.compile.src, mem.compile.token) == ERR) {
        write(STDERR_FILENO, "Error tokenizing source\n", 24);
        return ERR;
    }
    if (parse(mem.compile.token, &mem.compile) == ERR) {
        write(STDERR_FILENO, "Error parsing tokens\n", 21);
        return ERR;
    }
    return OK;
}

int main() {
    if (compile() == ERR) {
        write(STDERR_FILENO, "Compilation failed\n", 19);
        return 1;
    }
    return 0;
}