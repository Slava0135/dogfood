#pragma once

#include <cstddef>
#include <string>

void* align_alloc(std::size_t size);
char* path_join(const char *prefix, const char *file_name);
std::string rand_string(std::size_t len);
