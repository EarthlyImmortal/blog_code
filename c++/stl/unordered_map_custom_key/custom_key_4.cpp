#include <cstddef>
#include <iostream>
#include <unordered_map>

template <class T>
inline void HashCombine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    // 0x9e3779b97f4a7c15 是黄金分割比的 64 位定点表示：2^64 / φ
    seed ^= hasher(v) + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
}

struct Point
{
    int x;
    int y;
};

struct PointHash
{
    std::size_t operator()(const Point& p) const noexcept
    {
        std::size_t seed = 0;
        HashCombine(seed, p.x);
        HashCombine(seed, p.y);
        return seed;
    }
};

struct PointEqual
{
    bool operator()(const Point& a, const Point& b) const noexcept
    {
        return a.x == b.x && a.y == b.y;
    }
};

int main()
{
    std::unordered_map<Point, std::string, PointHash, PointEqual> m;
    m[{1, 2}] = "A";
    m[{3, 4}] = "B";
    std::cout << m[{1, 2}] << ", " << m[{3, 4}] << "\n";
    return 0;
}

/*
g++ -std=c++17 custom_key_4.cpp -o custom_key_4
*/