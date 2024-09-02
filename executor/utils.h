#pragma once

#include <cstddef>
#include <string>

void* align_alloc(std::size_t size);
std::string path_join(const std::string& prefix, const std::string& file_name);
std::string rand_string(std::size_t len);
