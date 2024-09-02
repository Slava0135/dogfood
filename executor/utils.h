#pragma once

#include <cstddef>

void* align_alloc(std::size_t size);
char* path_join(const char *prefix, const char *file_name);
char* rand_string(std::size_t len);
