#pragma once
#include <string>

#include "lz4/lz4.h"

std::string CompressWithLZ4(const std::string& input, int* compressed_size_out);
std::string DecompressWithLZ4(const std::string& compressed, int original_size);