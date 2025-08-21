#include <iostream>
#include <bitset>
#include "Person.pb.h"

using namespace tutorial;
using namespace std;

int main()
{
    Person p1;
    p1.set_id(128);
    p1.set_name("aaa");

    string output;
    p1.SerializeToString(&output);

    // 打印output的每一个字节
    for(const auto c : output) {
        //Q：为什么需要一种专门的数据结构表示位的数组(位图)
        cout << bitset<8>(c) << " ";
    }
    cout << endl;
}