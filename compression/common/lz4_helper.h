#pragma once
#include "lz4/lz4.h"
#include <string>

std::string CompressWithLZ4(const std::string& input, int* compressed_size_out);
std::string DecompressWithLZ4(const std::string& compressed, int original_size);