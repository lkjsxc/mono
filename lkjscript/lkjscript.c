#include "lkjscript.h"
#include "readsrc.c"
#include "token.c"

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
    return OK;
}

int main() {
    if (compile() == ERR) {
        write(STDERR_FILENO, "Compilation failed\n", 19);
        return 1;
    }
    for (token_t* itr = mem.compile.token; itr->data != NULL; itr++) {
        if (itr->data[0] == '\n') {
            write(STDOUT_FILENO, "\\n\n", 3);
            continue;
        } else {
            write(STDOUT_FILENO, itr->data, itr->size);
            write(STDOUT_FILENO, "\n", 1);
        }
    }
    return 0;
}