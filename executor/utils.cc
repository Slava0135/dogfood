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

const char* str_concat(const char *str_1, const char *str_2) {
    if (!str_1) {
        return strdup(str_2);
    }
    if (!str_2) {
        return strdup(str_1);
    }

    char *new_str = (char*)malloc(strlen(str_1) + strlen(str_2) + 1);
    if(new_str == nullptr){
        DPRINTF("ERROR: malloc failed\n");
        return nullptr;
    }
    //
    // ensures the memory is an empty string
    //
    new_str[0] = '\0';

    strcat(new_str, str_1);
    strcat(new_str, str_2);
    return new_str;
}

void str_trim(char *s) {
    if (!s) {
        return;
    }
    int pos = strlen(s) - 1;
    while (pos >= 0 && (s[pos] == ' ' || s[pos] == '\n')) {
        s[pos] = '\0';
        pos -= 1;
    }
}

// -----------------------------------------------

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

const char* exec_command(const char *cmd) {

    char *result = nullptr;
    char buf[128];

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        DPRINTF("ERROR: when executing `%s`\n", cmd);
        exit(1);
    }

    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        result = (char*)str_concat(result, buf);
    }
    pclose(fp);

    str_trim(result);
    return result;
}
