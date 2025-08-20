#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace std;

int main()
{
    ifstream in("output.json");
    if (!in.is_open())
    {
        std::cerr << "无法打开 output.json\n";
        return -1;
    }

    json obj;
    in >> obj;
    // 所有职位为"Developer"的员工
    cout << "职位为Developer的员工:" << endl;
    for (auto &employee : obj["employees"])
    {
        if (employee["position"] == "Developer")
            cout << employee["name"] << endl;
    }

    double sum = 0, ave;
    int count = 0;
    for (auto &employee : obj["employees"])
    {
        sum += employee["salary"].get<double>(); // 取出数值，需要用 .get<>() 或隐式转换
        ++count;
    }
    cout << "平均工资：" << sum / count << endl;

    cout << "掌握JavaScript技能的员工:" << endl;
    for (auto &employee : obj["employees"])
    {
        for (auto &skill : employee["skills"])
        {
            if (skill == "JavaScript")
                cout << employee["name"] << endl;
        }
    }
    return 0;
}
