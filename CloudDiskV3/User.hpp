#pragma once
#include <string>
using std::string;

struct User{
    int id;
    string username;
    string password;
    string salt;
    string createAt;
};

