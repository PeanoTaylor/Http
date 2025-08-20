#include "CryptoUtil.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
using namespace std;
static const char *SECRET_KEY = "my_secret_key";
string CryptoUtil::generateSalt(size_t length)
{
    static const char charset[] = "0123456789"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "abcdefghijklmnopqrstuvwxyz"
                                  "!@#$%^&*()-_=+[]{}|;:,.<>?";

    random_device rd;                                        // 随机种子数
    mt19937 gen(rd());                                       // 随机数生成
    uniform_int_distribution<> dist(0, sizeof(charset) - 2); // 均匀整数分布器模板

    string random_salt;
    random_salt.reserve();

    for (int i = 0; i < length; ++i)
    {
        random_salt.push_back(charset[dist(gen)]);
    }

    return random_salt;
}

string CryptoUtil::hashPassword(const string &password, const string &salt, const EVP_MD *md)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();  // 创建 EVP 上下文
    unsigned char hash[EVP_MAX_MD_SIZE]; // EVP_MAX_MD_SIZE: 最大哈希长度
    unsigned int hash_len;               // 用来接收实际的哈希长度

    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL); // 初始化上下文，采用 sha256 哈希算法
    string data = password + salt;
    EVP_DigestUpdate(ctx, data.c_str(), data.size()); // 更新上下文
    EVP_DigestFinal_ex(ctx, hash, &hash_len);         // 计算哈希值

    // 转换为十六进制字符串
    char result[2 * EVP_MAX_MD_SIZE + 1] = {'\0'};
    for (int i = 0; i < hash_len; i++)
    { // 转换成十六进制字符
        sprintf(result + 2 * i, "%02x", hash[i]);
    }
    EVP_MD_CTX_free(ctx); // 释放上下文
    return result;
}

string CryptoUtil::generateToken(const User &user, jwt_alg_t algorithm)
{
    jwt_t *jwt; // 创建jwt对象
    jwt_new(&jwt);

    jwt_set_alg(jwt, JWT_ALG_HS256, (unsigned char *)SECRET_KEY, strlen(SECRET_KEY));

    // 设置载荷(Payload): 用户自定义数据(不能存放敏感数据，比如：password, salt)
    jwt_add_grant(jwt, "sub", "login"); // 表示这个 Token 的用途是登录
    jwt_add_grant_int(jwt, "id", user.id);
    jwt_add_grant(jwt, "username", user.username.c_str());
    jwt_add_grant_int(jwt, "exp", time(NULL) + 1800); // 自定义过期时间字段

    char *token = jwt_encode_str(jwt);
    string result = token;

    jwt_free(jwt);
    free(token);

    return result;
}

bool CryptoUtil::verifyToken(const string &token, User &user)
{
    jwt_t *jwt;
    int err = jwt_decode(&jwt, token.c_str(), (unsigned char *)SECRET_KEY, strlen(SECRET_KEY));
    if (err)
    {

        return "failed";
    }
    jwt_free(jwt);
    return true;
}
