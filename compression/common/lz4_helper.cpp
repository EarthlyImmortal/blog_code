#include "lz4_helper.h"
#include <vector>

std::string CompressWithLZ4(const std::string& input, int* compressed_size_out)
{
    int max_dst_size = LZ4_compressBound(input.size());
    std::vector<char> compressed(max_dst_size);
    int compressed_size = LZ4_compress_default(input.data(), compressed.data(),
                                               input.size(), max_dst_size);
    if (compressed_size <= 0)
    {
        // 压缩失败，返回空字符串（调用者需处理）
        *compressed_size_out = 0;
        return "";
    }
    *compressed_size_out = compressed_size;
    return std::string(compressed.data(), compressed_size);
}

std::string DecompressWithLZ4(const std::string& compressed, int original_size)
{
    if (original_size <= 0)
        return "";
    std::vector<char> decompressed(original_size + 1, 0);
    int decompressed_size =
        LZ4_decompress_safe(compressed.data(), decompressed.data(),
                            compressed.size(), original_size);
    if (decompressed_size < 0)
    {
        return "";
    }
    return std::string(decompressed.data(), decompressed_size);
}