#pragma once
#include <string>
#include <map>
#include <iostream>
#include <strings.h>
#include <unistd.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/WFMySQLServer.h>
#include <workflow/MySQLResult.h>
#include <workflow/HttpMessage.h>
#include "CryptoUtil.hpp"
#include "User.hpp"
using std::string;
using namespace protocol;

bool checkAuth(HttpResponse *http_resp, HttpRequest *http_req, User &user);

void pread_callback(WFFileIOTask *task, HttpResponse *resp, string filename);

void register_callback(WFMySQLTask *task, HttpResponse *http_resp);

void login_callback(WFMySQLTask *task, HttpResponse *http_resp, string username, string password);