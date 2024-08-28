#include "utils.h"

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sched.h>
#include <cstddef>

#include "executor.h"

//
// Alignment: 4K
//
#define ALIGN 4096

void* align_alloc(std::size_t size) {
    void *ptr = nullptr;
    int ret = posix_memalign(&ptr, ALIGN, size);
    if (ret) {
      DPRINTF("ERROR: %s\n", strerror(errno));
    }
    return ptr;
}


//
// You must free the result if result is non-NULL.
// https://stackoverflow.com/questions/779875
//
char *str_replace(const char *orig, const char *rep, const char *with) {
    char *result; // the return string
    const char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return nullptr;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return nullptr; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = (char*)strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = (char*)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return nullptr;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
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

const char* str_format(char* fmt, ...) {
    const int BUF_LEN = 1024;
    char *buf = (char*)malloc(BUF_LEN);

    va_list vl;
    va_start(vl, fmt);

    int len = vsnprintf(buf, BUF_LEN, fmt, vl);
    if (len >= BUF_LEN) {
        DPRINTF("ERROR: string too long");
        free(buf);
        return nullptr;
    }

    va_end(vl);
    return buf;
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
