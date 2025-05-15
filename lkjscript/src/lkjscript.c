#include "lkjscript.h"
#include "compile/readsrc.c"

char src[1000];

result_t compile() {
    if(readsrc(SRC_PATH, src, sizeof(src)) == ERR) {
        write(STDERR_FILENO, "Error reading source file\n", 26);
        return ERR;
    }
    return OK;
}

int main() {
    if (compile() == ERR) {
        write(STDERR_FILENO, "Compilation failed\n", 19);
        return 1;
    }
    write(STDOUT_FILENO, src, sizeof(src));
    return 0;
}