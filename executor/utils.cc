#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "utils.h"
#include "executor.h"

void* align_alloc(std::size_t size) {
    void *ptr = nullptr;
    int ret = posix_memalign(&ptr, ALIGN, size);
    if (ret) {
      DPRINTF("ERROR: %s\n", strerror(errno));
    }
    return ptr;
}

char* path_join(const char *prefix, const char *file_name) {
    int len = strlen(prefix) + strlen(file_name) + 3;
    char *buf = (char*)malloc(len);
    int ret = snprintf(buf, len, "%s/%s", prefix, file_name);
    if (ret == len) {
        free(buf);
        DPRINTF("ERROR: when joining path\n");
        return nullptr;
    } else {
        buf[ret] = '\0';
    }
    return buf;
}

char* rand_string(std::size_t len) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "0123456789";
    if (!len) {
        return nullptr;
    }

    char *buf = (char*)align_alloc(len + 1);
    if (!buf) {
        return nullptr;
    }

    for (std::size_t i = 0; i < len; i++) {
        int key = rand() % (int) (sizeof(charset) -1);
        buf[i] = charset[key];
    }

    buf[len] = '\0';
    return buf;
}
