#include <iostream>
#include <map>
#include <string>
using std::cout;
using std::endl;
using std::map;
using std::string;

int main()
{
    // 结构化绑定
    map<string, int> info = {{"Alice", 90},
                             {"Bob", 85},
                             {"Charlie", 92}};

    for (const auto &[name, score] : info)
    {
        cout << name << ":" << score << endl;
    }
    return 0;
}
