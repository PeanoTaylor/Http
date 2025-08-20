#include <iostream>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

int main()
{
    /* json obj;
    obj["name"] = "Ubuntu";
    obj["version"] = "24.04";
    obj["skills"] = {"C++", "Linux", "Networking"};
    obj["active"] = true;
    cout << obj.dump(4) << endl; */
    
    //解析
    std::string text = R"({"name":"Ubuntu","version":24.04})";
    json obj = json::parse(text);

    std::cout << "Name: " << obj["name"] << std::endl;
    std::cout << "Version: " << obj["version"] << std::endl;
    return 0;
}
