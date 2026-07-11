#include <functional>  // std::hash
#include <iostream>
#include <string>
#include <unordered_map>

struct Point
{
    int x;
    int y;
};

// 在 std 命名空间中特化 std::hash
// 注意, 根据 Effective C++ 第25条: 为"用户定义类型"进行std templates
// 全特化是好的，但千万不要尝试在std内加入某些对std而言全新的东西
namespace std
{
template <>
struct hash<Point>
{
    std::size_t operator()(const Point& p) const noexcept
    {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);  // 简单组合
    }
};
}  // namespace std

// 定义operator==
bool operator==(const Point& a, const Point& b)
{
    return a.x == b.x && a.y == b.y;
}

int main()
{
    std::unordered_map<Point, std::string> m;
    m[{1, 2}] = "A";
    m[{3, 4}] = "B";
    std::cout << m[{1, 2}] << ", " << m[{3, 4}] << "\n";
    return 0;
}
/*
g++ -std=c++17 custom_key_1.cpp -o custom_key_1
*/