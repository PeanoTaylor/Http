#include <iostream>
#include "Person.pb.h" // 包含生成的头文件
#include <fstream>
#include <string>
using std::cout;
using std::endl;
using namespace tutorial;

int main()
{
    Person person;
    person.set_id(123);
    person.set_name("Simon");
    person.set_email("xxx@email.com");

    std::string output;

    person.SerializeToString(&output); // 序列化
    cout << "size: " << output.size() << endl;
    cout << "output: " << output << endl;

    // 反序列化
    Person new_person;
    cout << "new_person.name: " << new_person.name()
         << ", new_person.id: " << new_person.id()
         << ", new_person.email: " << new_person.email() << endl;

    new_person.ParseFromString(output);
    cout << "new_person.name: " << new_person.name() // getter
         << ", new_person.id: " << new_person.id()
         << ", new_person.email: " << new_person.email() << endl;
    return 0;
}
