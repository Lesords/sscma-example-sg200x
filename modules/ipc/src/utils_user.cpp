#include <iostream>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>

#include "HttpServer.h"
#include "global_cfg.h"
#include "utils_user.h"

static std::string g_sUserName;
static std::string g_sPassword;

static int getUserName()
{
    char username[16];
    struct passwd* pw = getpwuid(getuid());

    if (pw) {
        strncpy(username, pw->pw_name, sizeof(username));
    } else {
        perror("getpwuid");
        return -1;
    }

    g_sUserName.assign(username);
    return 0;
}

int queryUserInfo(HttpRequest* req, HttpResponse* resp)
{
    hv::Json User;
    User["code"] = 0;
    User["msg"] = "";

    getUserName();
    printf("%s, %s\n", g_sUserName.c_str(), g_sPassword.c_str());

    hv::Json data;
    data["userName"] = g_sUserName;
    data["password"] = g_sPassword;

    FILE* fp;
    char info[128];
    std::vector<hv::Json> sshkeyList;

    fp = popen(SCRIPT_USER_SSH, "r");
    if (fp == NULL) {
        printf("Failed to run %s\n", SCRIPT_USER_SSH);
    }

    while (fgets(info, sizeof(info) - 1, fp) != NULL) {
        std::string s(info);
        hv::Json sshkey;
        if (s.back() == '\n') {
            s.erase(s.size() - 1);
        }

        sshkey["id"] = "-";
        sshkey["name"] = "-";
        sshkey["value"] = s;
        sshkey["addTime"] = "20240101";
        sshkey["latestUsedTime"] = "20240101";
        sshkeyList.push_back(sshkey);
    }
    pclose(fp);

    data["sshkeyList"] = hv::Json(sshkeyList);
    User["data"] = data;

    return resp->Json(User);
}

int updateUserName(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nupdate UserName operation...\n";
    std::cout << "userName: " << req->GetString("userName") << "\n";

    FILE* fp;
    char cmd[128] = SCRIPT_USER_NAME;

    strcat(cmd, g_sUserName.c_str());
    strcat(cmd, " ");
    strcat(cmd, req->GetString("userName").c_str());
    printf("cmd: %s\n", cmd);

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run %s list\n", cmd);
        return -1;
    }
    pclose(fp);

    hv::Json User;
    User["code"] = 0;
    User["msg"] = "";
    User["data"] = hv::Json({});

    return resp->Json(User);
}

int updatePassword(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nupdate Password operation...\n";
    // std::cout << "oldPassword: " << req->GetString("oldPassword") << "\n";
    // std::cout << "newPassword: " << req->GetString("newPassword") << "\n";

    FILE* fp;
    char info[128];
    char cmd[128] = SCRIPT_USER_PWD;

    strcat(cmd, req->GetString("oldPassword").c_str());
    strcat(cmd, " ");
    strcat(cmd, req->GetString("newPassword").c_str());
    printf("cmd: %s\n", cmd);

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run %s list\n", cmd);
        return -1;
    }

    hv::Json User;
    User["code"] = 0;
    User["msg"] = "";
    User["data"] = hv::Json({});

    if (fgets(info, sizeof(info) - 1, fp) != NULL) {
        printf("info: =%s=\n", info);
        if (strcmp(info, "OK\n") != 0) {
            printf("error here\n");
            User["code"] = 1109;
            if (fgets(info, sizeof(info) - 1, fp) != NULL) {
                printf("error info: %s\n", info);
                User["msg"] = std::string(info);
            }
        }
    }
    pclose(fp);

    return resp->Json(User);
}

int addSShkey(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nAdd ssh key operation...\n";
    std::cout << "name:" << req->GetString("name") << "\n";
    std::cout << "value: " << req->GetString("value") << "\n";

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";

    hv::Json data;
    data["id"] = "666";
    response["data"] = data;

    return resp->Json(response);
}

int deleteSShkey(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\ndelete ssh key operation...\n";
    std::cout << "id: " << req->GetString("id") << "\n";

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}
