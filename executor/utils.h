#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstddef>

void* align_alloc(std::size_t size);
char *str_replace(const char *orig, const char *rep, const char *with);
const char* str_concat(const char *str_1, const char *str_2);
void str_trim(char *s);
const char* str_format(char* fmt, ...);
void make_dir(const char *dir_path);
char* path_join(const char *prefix, const char *file_name);
char* rand_string(std::size_t len);
const char* exec_command(const char *cmd);

#endif /* ifndef __UTILS_H__ */
