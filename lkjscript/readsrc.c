#include "lkjscript.h"

result_t readsrc(const char *filename, char* dst, int64_t max_size) {
    int64_t fd = open(filename, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error opening file\n", 19);
        return ERR;
    }
    int64_t n = read(fd, dst, max_size - 2);
    if (n < 0) {
        close(fd);
        write(STDERR_FILENO, "Error reading file\n", 19);
        return ERR;
    }
    dst[n + 0] = '\n';
    dst[n + 1] = '\0';
    close(fd);
    return OK;
}
