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

std::string path_join(const std::string& prefix, const std::string& file_name) {
    return prefix + "/" + file_name;
}

std::string rand_string(std::size_t len) {
    auto randchar = []() -> char {
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
