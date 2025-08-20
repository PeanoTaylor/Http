#include <iostream>
#include "wfrest/HttpServer.h"
#include <string>
#include <vector>
#include <sstream>
#include <map>
using namespace std;
using namespace wfrest;

map<string, string> prams_reslove(const string &segment)
{
    map<string, string> res;
    auto split_flag = segment.find(';'); // 找到第一个分号
    if (split_flag == string::npos)
    {
        return res; // 没有找到分隔符号
    }

    string segment_substr = segment.substr(split_flag + 1);
    istringstream iss{segment_substr};
    string pram;
    while (getline(iss, pram, ';'))
    {
        auto equal = segment_substr.find("=");
        res[pram.substr(0, equal)] = res[pram.substr(equal + 1)];
    }
    return res;
}

int main()
{
    HttpServer server; // 注册HttpServer对象

    // 注册GET路由
    server.GET("/*", [](const HttpReq *req, HttpResp *resp)
               {
                   string path = req->current_path();
                   //cout << path << endl;

                   vector<map<string, string>> matrixParams;
                   istringstream iss{path};
                   string segment;
                   while (getline(iss, segment, '/'))
                   {
                       if (!segment.empty())
                       {
                           map<string, string> prams = prams_reslove(segment);
                           matrixParams.emplace_back(prams);
                       }
                   }
                   for (int i = 0; i < matrixParams.size(); ++i)
                   {
                        cout << i << ":";
                       for (const auto &[name, value] : matrixParams[i])
                       {
                           cout << name << ":" << value;
                       }
                       cout<<endl;
                   } });

    // 监听端口并运行
    if (server.start(8888) == 0)
    {
        getchar();
        server.stop();
    }
    return 0;
}
