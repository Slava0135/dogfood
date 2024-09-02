#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <string>
#include <algorithm>

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

std::string rand_string(std::size_t len) {
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const std::size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(len, 0);
    std::generate_n(str.begin(), len, randchar);
    return str;
}
