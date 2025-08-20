#include <iostream>
#include <assert.h>
using std::cout;
using std::endl;

void fx(int value){
    assert(value > 0 && "value must > 0");
}

int main()
{
    fx(5);
    fx(-1);//编译时添加-DNDEBUG，assert语句被编译器移除，程序正常输出
    return 0;
}

