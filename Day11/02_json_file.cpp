#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

int main()
{
    // 写入json文件
    json obj;
    obj["name"] = "Alice";
    obj["age"] = 18;

    ofstream out("output.json");
    out << obj.dump(4) << endl;

    // 读取json文件
    ifstream in("output.json");
    if (!in.is_open())
    {
        std::cerr << "无法打开 output.json\n";
        return 1;
    }
    json out_put;
    in >> out_put;
    cout << obj.dump(4) << endl;
    return 0;
}
