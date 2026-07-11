#include <iostream>
#include <string>
#include <unordered_map>

struct Point
{
    int x;
    int y;
};

int main()
{
    auto hash = [](const Point& p)
    {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);  // 简单组合
    };
    auto equal = [](const Point& a, const Point& b)
    { return a.x == b.x && a.y == b.y; };

    // C++17 写法：必须显式传入 lambda 实例
    std::unordered_map<Point, std::string, decltype(hash), decltype(equal)> m(
        13 /*bucket 提示*/, hash, equal);

    // C++20写法: 无需再把 lambda 传给构造函数
    // std::unordered_map<Point, std::string, decltype(hash), decltype(equal)>
    // m;

    m[{1, 2}] = "A";
    m[{3, 4}] = "B";
    std::cout << m[{1, 2}] << ", " << m[{3, 4}] << "\n";
    return 0;
}

/*
g++ -std=c++17 custom_key_3.cpp -o custom_key_3
g++ -std=c++20 custom_key_3.cpp -o custom_key_3
*/