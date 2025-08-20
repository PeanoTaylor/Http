#include "CloudiskServer.h"
#include "CryptoUtil.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <workflow/MySQLResult.h>
#include <nlohmann/json.hpp>
#include <wfrest/HttpMsg.h>
#include <map>
#include <vector>
#include <wfrest/HttpFile.h>
#include <wfrest/PathUtil.h>

using std::cout;
using std::endl;
using std::map;
using std::vector;
using std::string;
using std::ofstream;
using namespace wfrest;
using namespace protocol;
using json = nlohmann::json;
namespace fs = std::filesystem;

static WFFacilities::WaitGroup g_wait_group(1);
static const string mysql_url = "mysql://root:ZYJ.01234@127.0.0.1/http";

void CloudiskServer::register_modules()
{
    // 设置静态资源的路由
    register_static_resources_module();
    register_signup_module();
    register_signin_module();
    register_userinfo_module();
    register_filelist_module();
    register_fileupload_module();
    register_filedownload_module();
    register_filedelete_module();
    cout << "start success" << endl;
}

string urlDecode(const string &src) {
    std::string ret;
    char ch;
    int i, ii;
    for (i = 0; i < (int)src.length(); i++) {
        if (int(src[i]) == 37) {  // %
            sscanf(src.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        } else if (src[i] == '+') {
            ret += ' ';  // '+' 转空格
        } else {
            ret += src[i];
        }
    }
    return ret;
}

map<string, string> parse_uri(const string &uri) {
    map<string, string> params;
    size_t pos = uri.find("?");
    if (pos == string::npos) return params;

    string query = uri.substr(pos + 1);
    size_t start = 0;
    while (start < query.size()) {
        size_t eq = query.find("=", start);
        if (eq == string::npos) break;
        string key = query.substr(start, eq - start);

        size_t amp = query.find("&", eq + 1);
        string value;
        if (amp == string::npos) {
            value = query.substr(eq + 1);
            params[key] = urlDecode(value);   // ✅ decode
            break;
        } else {
            value = query.substr(eq + 1, amp - eq - 1);
            params[key] = urlDecode(value);   // ✅ decode
            start = amp + 1;
        }
    }
    return params;
}


void CloudiskServer::register_static_resources_module()
{
    m_server.GET("/user/signup", [](const HttpReq *, HttpResp *resp)
                 { resp->File("static/view/signup.html"); });

    m_server.GET("/static/view/signin.html", [](const HttpReq *, HttpResp *resp)
                 { resp->File("static/view/signin.html"); });

    m_server.GET("/static/view/home.html", [](const HttpReq *, HttpResp *resp)
                 { resp->File("static/view/home.html"); });

    m_server.GET("/static/js/auth.js", [](const HttpReq *, HttpResp *resp)
                 { resp->File("static/js/auth.js"); });

    m_server.GET("/static/img/avatar.jpeg", [](const HttpReq *, HttpResp *resp)
                 { resp->File("static/img/avatar.jpeg"); });

    m_server.GET("/file/upload", [](const HttpReq *, HttpResp *resp)
                 { resp->File("static/view/index.html"); });

    m_server.Static("/file/upload_files", "static/view/upload_files");
}

// 注册模块
void CloudiskServer::register_signup_module() {
    // GET: 返回登录页面
    m_server.GET("/user/signin", [](const HttpReq * , HttpResp * resp) {
        resp -> File("static/view/signin.html");
    });

    // POST: 处理登录请求
    m_server.POST("/user/signup", [](const HttpReq * req, HttpResp * resp, SeriesWork * series) {
        // 1. 校验请求参数，解析请求参数
        if (req -> content_type() != APPLICATION_URLENCODED) {
            resp -> set_status(HttpStatusBadRequest);
            resp -> String("<html>400 Bad Request</html>");
            return;
        }

        auto & params = req -> form_kv();
        string username = urlDecode(params["username"]);
        string password = params["password"];
        string passworddc = params["passwordc"];

        if (username.empty() || password.empty()) {
            resp -> set_status(HttpStatusBadRequest);
            json j;
            j["status"] = "Missing username or password";
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4)); 
            return;
        }

        // 2.生成盐值
        string salt = CryptoUtil::generateSalt();
        string hash = password + salt;

        string store_password = CryptoUtil::hashPassword(password, salt, EVP_sha256());

        string sql_signup = "INSERT INTO tbl_user(username, password, salt) VALUES ('" + username + "', '" + store_password + "', '" + salt + "')";

        resp -> MySQL(mysql_url, sql_signup, [resp](MySQLResultCursor * cursor) { // 适合 INSERT/UPDATE/DELETE 或者需要手动遍历结果的场景
            if (!cursor) {
                resp -> set_status(HttpStatusBadRequest);
                json j;
                j["status"] = "insert failed, username exists";
                resp -> Json(j.dump(4));
                return;
            }
            resp -> set_status(HttpStatusOK);
            json j;
            j["status"] = "SUCCESS";
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4)); 
        });

        // 2. 业务逻辑 (任务之间是串行的，还行并行的)
        // a. 创建MySQL任务，添加到序列中
        // b. resp->MySQL(url, sql, jsonHanlder);
        // c. resp->MySQL(url, sql, cursorHandler);

        // 3. 生成响应
    });
}

// 登录模块
void CloudiskServer::register_signin_module()
{
    // seriesHandler
    m_server.POST("/user/signin", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
                  {
        // 1. 校验请求参数，解析请求参数
        if (req -> content_type() != APPLICATION_URLENCODED) {
            resp -> set_status(HttpStatusBadRequest);
            resp -> String("<html>400 Bad Request</html>");
        }

        auto & params = req -> form_kv();
        string username = urlDecode(params["username"]);
        string password = params["password"];

        // 2.查询用户是否存在
        string sql_signin = "SELECT id, password, salt FROM tbl_user WHERE username = '" + username + "' LIMIT 1;";

        resp->MySQL(mysql_url, sql_signin, [resp,username,password](MySQLResultCursor* cursor) {//适合 INSERT/UPDATE/DELETE 或者需要手动遍历结果的场景
            // 查询没有结果
            map <string, MySQLCell> record;
            if (!cursor->fetch_row(record)) {
                resp->set_status(HttpStatusBadRequest);
                json j;
                j["status"] = "select failed, username not exists";
                resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
                resp->String(j.dump(4)); 
                return;
            }

            // 查询成功
            User user;
            user.id = record["id"].as_int();
            user.username = username;
            user.password = record["password"].as_string();
            user.salt = record["salt"].as_string();

            // 计算页面获取的密码+数据库存储salt的哈希值是否相等
            string cal_hash = CryptoUtil::hashPassword(password,user.salt,EVP_sha256());
            if(cal_hash != user.password){
                resp->set_status(HttpStatusUnauthorized);
                json j;
                j["status"] = "error";
                j["msg"] = "invalid password";
                resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
                resp->String(j.dump(4)); 
                return;
            }

            // 登录成功
            // 生成token
            string token  = CryptoUtil::generateToken(user,JWT_ALG_HS256);

            // 构造返回 JSON
            resp->set_status(HttpStatusOK);
            json j;
            j["data"]["username"] = user.username;
            j["data"]["token"] = token;
            j["data"]["location"] = "/static/view/home.html";
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4)); 

            cout << "login success, user=" << user.username << endl;
        }); });
}

// 获取用户信息
void CloudiskServer::register_userinfo_module(){
    m_server.GET("/user/info",[](const HttpReq *req, HttpResp *resp){
        // 解析uri
        string uri = req->get_request_uri();
        auto params = parse_uri(uri);

        User user;
        user.username = params["username"];
        string token = params["token"];

        if (!CryptoUtil::verifyToken(token, user)) {
            resp->set_status(HttpStatusUnauthorized);
            json j;
            j["status"] = "invalid or expired token";
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4));
            return;
        }

        
        // 4.校验token
        if(!CryptoUtil::verifyToken(token,user)){
            resp->set_status(HttpStatusUnauthorized);   // 401
            json j;
            j["status"] = "Unauthorized";
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4)); 
            return;
        }
        // 5.查询用户注册时间
        string sql_created_at = "SELECT created_at FROM tbl_user WHERE username = '" + user.username + "';";

        resp->MySQL(mysql_url,sql_created_at,[resp,user](MySQLResultCursor* cursor){
            map<string,MySQLCell> record;
            if(!cursor->fetch_row(record)){
                resp->set_status(HttpStatusBadRequest);
                json j;
                j["status"] = "user not exists";
                resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
                resp->String(j.dump(4)); 
                return;
            }

            // 用户已经注册
            //user.createAt = record["created_at"].as_string();

            string createAt = record["created_at"].as_string();

            // 6.构造返回JSON
            json j;
            j["data"]["Username"] = user.username;
            j["data"]["SignupAt"] = createAt;
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4)); 
        });
    });
}

// 文件列表查询
void CloudiskServer::register_filelist_module(){
    m_server.POST("/file/query",[](const HttpReq *req, HttpResp *resp){
        // 解析uri
        string uri = req->get_request_uri();
        auto params = parse_uri(uri);

        User user;
        user.username = params["username"];
        string token = params["token"];

        if (!CryptoUtil::verifyToken(token, user)) {
            resp->set_status(HttpStatusUnauthorized);
            json j;
            j["status"] = "invalid or expired token";
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4));
            return;
        }
        
        // 2.从表单获取list
        auto &list = req->form_kv();   // x-www-form-urlencoded
        int limit = 10;


        // 4.查询当前用户文件列表
        string sql_file_query = "SELECT f.hashcode, f.filename, f.size, f.created_at, f.last_update FROM tbl_file f "
        "LEFT JOIN tbl_user u on u.id = f.uid "
        "WHERE u.username = '" + user.username + "' AND f.status = 0 "
        "ORDER BY f.created_at DESC "
        "LIMIT " + std::to_string(limit) + ";";

        resp->MySQL(mysql_url,sql_file_query,[resp](MySQLResultCursor* cursor){
            vector<json> fileInfo;
            map<string,MySQLCell> record;
            
            while(cursor->fetch_row(record)){
                json f;
                f["FileHash"] = record["hashcode"].as_string();
                f["FileName"] = record["filename"].as_string();
                f["FileSize"]    = record["size"].as_ulonglong();
                f["UploadAt"] = record["created_at"].as_string();
                f["LastUpdated"] = record["last_update"].as_string();
                fileInfo.push_back(f);
            }


            if(fileInfo.empty()){
                resp->set_status(HttpStatusOK);
                json j;
                j["status"] = "file not exists";
                resp->String(j.dump(4));
                return;
            }

            // 构造返回JSON
            resp->set_status(HttpStatusOK);
            resp->String(json(fileInfo).dump(4));
        });
    });
}

// 文件上传
void CloudiskServer::register_fileupload_module() {
    m_server.POST("file/upload", [](const HttpReq * req, HttpResp * resp) {

        // 1.解析uri
        string uri = req -> get_request_uri();
        auto params = parse_uri(uri);
        User user;
        user.username = params["username"];
        string token = params["token"];

        // 2.根据用户名查询uid
        string sql_signin = "SELECT id FROM tbl_user WHERE username = '" + user.username + "' LIMIT 1;";

        resp -> MySQL(mysql_url, sql_signin, [resp, req, user](MySQLResultCursor * cursor) { //适合 INSERT/UPDATE/DELETE 或者需要手动遍历结果的场景
            // 查询没有结果
            map < string, MySQLCell > record;
            if (!cursor -> fetch_row(record)) {
                resp -> set_status(HttpStatusBadRequest);
                json j;
                j["status"] = "select failed, username not exists";
                resp -> add_header_pair("Content-Type", "application/json");
                resp -> String(j.dump(4));
                return;
            }
            // 查询成功
            int uid = record["id"].as_int();


            // 3.读取文件上传内容(以 form-data 的方式上传文件
            if (req -> content_type() != MULTIPART_FORM_DATA) {
                resp -> set_status(HttpStatusBadRequest); // 响应状态码: 400 BadRequest
                return;
            }
            // 4.解析表单，
            const Form & form = req -> form();

            for (const auto & [_, fileInfo]: form) {
                const auto & [filename, content] = fileInfo;

                // 写入磁盘,wfrest自带
                //resp -> Save(PathUtil::base(filename), std::move(content));

                // 计算文件哈希（基于磁盘路径）
                string content_hash = CryptoUtil::hashFile(content, EVP_sha256());

                // 6.写入数据库

                // 先查tbl_file是否已有content_hash记录
                // 如果有，则不用存到文件目录下，只需要往表中插入记录即可
                // 如果没有，则需要将文件保存到文件目录下，并且将记录写入文件表

                // 5.先查该用户是否已有同名文件
                string sql_check_filename = "SELECT id, filename FROM tbl_file WHERE uid=" + std::to_string(uid) +
                    " AND filename='" + filename + "' LIMIT 1";


                // string sql_check = "SELECT id FROM tbl_file WHERE hashcode ='" + content_hash +"' LIMIT 1";

                resp -> MySQL(mysql_url, sql_check_filename, [resp, uid, filename, content_hash, size = content.size(), content](MySQLResultCursor * filename_cursor) {
                    map < string, MySQLCell > filename_record;
                    if (filename_cursor -> fetch_row(filename_record)) {
                        // 已有同名文件,但是内容不一样 → 更新记录
                        string sql_update_hashcode = "UPDATE tbl_file SET hashcode='" + content_hash +
                            "', size='" + std::to_string(size) + "' WHERE filename= '" + filename_record["filename"].as_string() +"';";
                        // 同名新文件保存到本地
                        string save_path = "./static/view/upload_files/" + PathUtil::base(content_hash);
                        ofstream ofs(save_path, std::ios::binary);
                        if (!ofs) {
                            resp -> set_status(HttpStatusInternalServerError);
                            json j;
                            j["status"] = "failed to save file";
                            resp -> add_header_pair("Content-Type", "application/json");
                            resp -> String(j.dump(4));
                            return;
                        }
                        ofs.write(content.data(), content.size());
                        ofs.close();

                        // 上传到OSS
                        /* OssClientWrapper oss{"",
                                            "",
                                            ""
                                            };

                        oss.putObject("peano-taylor", "dir/" + filename , "./static/view/upload_files/" + content_hash); */ 


                        resp -> MySQL(mysql_url, sql_update_hashcode, [resp, uid, filename, content_hash, size = content.size(), content](MySQLResultCursor * update_hashcode_cursor) {
                            json j;
                            j["filename"] = filename;
                            if (update_hashcode_cursor->get_cursor_status() == MYSQL_STATUS_OK) {
                                resp->set_status(HttpStatusSeeOther); // 303
                                resp->add_header_pair("Location", "./static/view/home.html");
                            } else {
                                resp->set_status(HttpStatusInternalServerError);
                                j["status"] = "update failed";
                                resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
                                resp->String(j.dump(4));
                            }
                        });
                    } else { // 没有同名文件
                        // 检查文件目录下是否已存在该文件(遍历tbl_file的hashcode)
                        string sql_check_hashcode = "SELECT id FROM tbl_file WHERE hashcode = '" + content_hash + "';";
                        resp -> MySQL(mysql_url, sql_check_hashcode, [resp, uid, filename, content_hash, size = content.size(), content](MySQLResultCursor * check_hashcode_cursor) {
                            map < string, MySQLCell > check_hashcode_record;
                            if (check_hashcode_cursor -> fetch_row(check_hashcode_record)) {
                                // tbl_file中已存在hashcode → 表中插入记录
                                string sql_insert_file_info = "INSERT INTO tbl_file(uid, filename, hashcode, size) VALUES('" +
                                    std::to_string(uid) + "','" + filename + "','" + content_hash + "','" + std::to_string(size) + "')";

                                resp -> MySQL(mysql_url, sql_insert_file_info, [resp, filename](MySQLResultCursor * update_cursor) {
                                    json j;
                                    j["filename"] = filename;
                                    if (update_cursor -> get_cursor_status() == MYSQL_STATUS_OK) {
                                        //resp->set_status(HttpStatusOK);
                                        //j["status"] = "update success";
                                        resp -> set_status(HttpStatusSeeOther); // 303
                                        resp -> add_header_pair("Location", "./static/view/home.html");
                                    } else {
                                        resp -> set_status(HttpStatusInternalServerError);
                                        j["status"] = "update failed";
                                    }
                                    resp -> add_header_pair("Content-Type", "application/json");
                                    resp -> String(j.dump(4));
                                });

                            } else {
                                string sql_insert_file_info = "INSERT INTO tbl_file(uid, filename, hashcode, size) VALUES('" +
                                    std::to_string(uid) + "','" + filename + "','" + content_hash + "','" + std::to_string(size) + "')";

                                // 文件目录中不存在则写入
                                string save_path = "./static/view/upload_files/" + PathUtil::base(content_hash);
                                ofstream ofs(save_path, std::ios::binary);
                                if (!ofs) {
                                    resp -> set_status(HttpStatusInternalServerError);
                                    json j;
                                    j["status"] = "failed to save file";
                                    resp -> add_header_pair("Content-Type", "application/json");
                                    resp -> String(j.dump(4));
                                    return;
                                }
                                ofs.write(content.data(), content.size());
                                ofs.close();

                                // 上传到OSS
                                /* OssClientWrapper oss{"",
                                                    "",
                                                    ""
                                                    };

                                oss.putObject("peano-taylor", "dir/" + filename , "./static/view/upload_files/" + content_hash); */ 


                                resp -> MySQL(mysql_url, sql_insert_file_info, [resp, filename](MySQLResultCursor * update_cursor) {
                                    json j;
                                    j["filename"] = filename;
                                    if (update_cursor -> get_cursor_status() == MYSQL_STATUS_OK) {
                                        //resp->set_status(HttpStatusOK);
                                        //j["status"] = "update success";
                                        resp -> set_status(HttpStatusSeeOther); // 303
                                        resp -> add_header_pair("Location", "./static/view/home.html");
                                    } else {
                                        resp -> set_status(HttpStatusInternalServerError);
                                        j["status"] = "update failed";
                                    }
                                    resp -> add_header_pair("Content-Type", "application/json");
                                    resp -> String(j.dump(4));
                                });
                            }
                        });
                    }

                });

            }
        });
    });

    

    m_server.Static("/file/static/view", "./static/view");
}

// 文件下载
void CloudiskServer::register_filedownload_module(){
    m_server.GET("file/download",[](const HttpReq* req,HttpResp *resp){

        auto params = req->query_list();
        string filename = params["filename"];
        string hashcode = params["filehash"];
        string username = params["username"];
        string token = params["token"];

        
        User user;

        if (!CryptoUtil::verifyToken(token, user)) {
            resp->set_status(HttpStatusUnauthorized);
            json j;
            j["status"] = "invalid or expired token";
            resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(j.dump(4));
            return;
        }
        cout << username << ":" << token << endl;
        // 根据username —> uid 关联 tbl_file的uid -> hashcode(路径下的文件)
        string sql_find_file ="SELECT f.filename, f.hashcode, f.size, f.last_update FROM tbl_file f "
            "LEFT JOIN tbl_user u ON u.id = f.uid "
            "WHERE u.username = '" + urlDecode(username) + "' "
            "AND f.hashcode = '" + hashcode + "' LIMIT 1;";
        resp->MySQL(mysql_url,sql_find_file,[resp,filename,hashcode](MySQLResultCursor* cursor){
            map <string,MySQLCell> file_record;
            if (!cursor->fetch_row(file_record)) {
                resp->set_status(HttpStatusNotFound);
                json j;
                j["status"] = "file not found";
                resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
                resp->String(j.dump(4));
                return;
            }

            
            string path = "./static/view/upload_files/" + hashcode;
            // 找到文件
            if (access(path.c_str(), R_OK) == -1) {
                resp->set_status(HttpStatusNotFound);
                json j;
                j["status"] = "file missing on server";
                resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
                resp->String(j.dump(4));
                return;
            }

            // 设置下载响应头
            resp->set_status(HttpStatusOK);
            resp->add_header_pair("Content-Disposition", "attachment; filename=\"" + filename + "\"");
            resp->File(path);

        });
    });
}

// 文件删除
void CloudiskServer::register_filedelete_module() {
    m_server.POST("/file/delete", [](const HttpReq * req, HttpResp * resp) {
        std::cout << "进入 /file/delete handler" << std::endl;
        string hashcode = req -> form_kv()["filehash"];
        string filename = urlDecode(req -> form_kv()["filename"]);
        string username = urlDecode(req -> query("username"));

        if (hashcode.empty() || filename.empty() || username.empty()) {
            json j;
            j["status"] = "FAIL";
            j["msg"] = "参数缺失";
            resp -> String(j.dump(4));
            return;
        }

        resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
        // Step1: 查 uid
        string sql_uid = "SELECT id FROM tbl_user WHERE username='" + username + "';";
        resp -> MySQL(mysql_url, sql_uid,
            [resp, hashcode, filename, username](MySQLResultCursor * uid_cursor) {
                map < string, MySQLCell > record;
                if (!uid_cursor -> fetch_row(record)) {
                    json j;
                    j["status"] = "FAIL";
                    j["msg"] = "用户不存在";
                    resp -> String(j.dump(4));
                    return;
                }

                int uid = record["id"].as_int();

                // Step2: 删除用户文件记录
                string sql_delete = "DELETE FROM tbl_file WHERE  hashcode='" + hashcode +
                    "' AND filename='" + filename + "' LIMIT 1;";
                    cout << sql_delete << endl;
                resp -> MySQL(mysql_url, sql_delete,
                    [resp, uid, hashcode](MySQLResultCursor * del_cursor) {

                        // Step3: 检查是否还剩同 hash 文件
                        string sql_check = "SELECT count(*) as cnt FROM tbl_file WHERE uid='" + std::to_string(uid) + "';";
                        resp -> MySQL(mysql_url, sql_check,
                            [resp, hashcode](MySQLResultCursor * check_cursor) {
                                map < string, MySQLCell > rec;
                                if (!check_cursor -> fetch_row(rec)) {
                                    json j;
                                    j["status"] = "FAIL";
                                    j["msg"] = "检查文件引用失败";
                                    resp -> String(j.dump(4));
                                    return;
                                }

                                int cnt = rec["cnt"].as_ulonglong();
                                if (cnt == 0) {
                                    // 物理文件没人用了 → 删除
                                    string filepath = "./static/view/upload_files/" + hashcode;
                                    fs::remove(filepath);
                                    json j;
                                    j["status"] = "SUCCESS";
                                    j["msg"] = "删除成功";
                                    resp -> String(j.dump(4));
                                } else {
                                    json j;
                                    j["status"] = "SUCCESS";
                                    j["msg"] = "删除成功";
                                    resp -> String(j.dump(4));
                                }
                            });
                    });
            });
    });
}
    
