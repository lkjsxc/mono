#include <unistd.h>

#include "lkjscript.h"

static mem_t mem;

int main() {
    write(1, "Good Morning, World!\n", 21);
    return 0;
}