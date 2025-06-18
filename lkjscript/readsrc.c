#include "lkjscript.h"

__attribute__((warn_unused_result))
result_t readsrc(const char *filename, char* dst, int64_t max_size) {
    int64_t fd = open(filename, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error opening file\n", 19);
        return ERR;
    }
    dst[0] = '\n';
    int64_t n = read(fd, dst + 1, max_size - 3);
    if (n < 0) {
        close(fd);
        write(STDERR_FILENO, "Error reading file\n", 19);
        return ERR;
    }
    dst[n + 1] = '\n';
    dst[n + 2] = '\0';
    close(fd);
    return OK;
}
