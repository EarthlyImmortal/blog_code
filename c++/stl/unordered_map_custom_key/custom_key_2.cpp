#include <cstddef>
#include <iostream>
#include <unordered_map>

struct Point
{
    int x;
    int y;
};

struct PointHash
{
    std::size_t operator()(const Point& p) const noexcept
    {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);  // 简单组合
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
g++ -std=c++17 custom_key_2.cpp -o custom_key_2
*/