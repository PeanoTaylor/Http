#pragma once
#include "User.hpp"
#include <string>
#include <iostream>
#include <random>
#include <openssl/evp.h>
#include "jwt.h"
using std::string;
using std::cout;
using std::endl;
class CryptoUtil
{
public:
    static string generateSalt(size_t length = 16);
    static string hashPassword(const string &password, const string &salt, const EVP_MD *md);
    static string hashFile(const string &fileData, const EVP_MD *md);
    static string generateToken(const User &user, jwt_alg_t algorithm = JWT_ALG_HS256);
    static bool verifyToken(const string &token, User &user);

private:
    CryptoUtil() = delete;
};
