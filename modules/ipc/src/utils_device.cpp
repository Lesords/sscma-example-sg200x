#include <fstream>
#include <iostream>
#include <iterator>
#include <stdio.h>
#include <string>

#include "HttpServer.h"
#include "global_cfg.h"
#include "utils_device.h"

int g_progress = 0;

std::string readFile(const std::string& path, const std::string& defaultname)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        // 文件无法打开，返回默认值
        return defaultname;
    }
    // 读取文件内容并返回
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

static std::string writeFile(const std::string& path, const std::string& strWrite)
{
    std::ofstream outfile(path);
    if (outfile.is_open()) {
        outfile << strWrite;
        outfile.close();
        std::cout << "Write Success: " << path << std::endl;
    }
}

int queryDeviceInfo(HttpRequest* req, HttpResponse* resp)
{
    hv::Json device;
    device["code"] = 0;
    device["msg"] = "";

    std::string os_version = readFile(PATH_ISSUE);
    std::string os = "Null", version = "Null";
    size_t pos = os_version.find(',');
    if (pos != std::string::npos) {
        os = os_version.substr(0, pos);
        version = os_version.substr(pos + 1);
    }

    std::string ch_url = readFile(PATH_UPGRADE_URL);
    std::string ch = "0", url = "";
    pos = ch_url.find(',');
    if (pos != std::string::npos) {
        ch = ch_url.substr(0, pos);
        url = ch_url.substr(pos + 1);
    }

    hv::Json data;
    data["deviceName"] = readFile(PATH_DEVICE_NAME);
    data["ip"] = req->host;
    data["mask"] = "255.255.255.0";
    data["gateway"] = "-";
    data["dns"] = "-";
    data["channel"] = std::stoi(ch);
    data["serverUrl"] = url;
    data["officialUrl"] = DEFAULT_UPGRADE_URL;
    data["cpu"] = "sg2002";
    data["ram"] = 128;
    data["npu"] = 2;
    data["osName"] = os;
    data["osVersion"] = version;
    data["osUpdateTime"] = "2024.01.01";
    data["terminalPort"] = TTYD_PORT;

    device["data"] = data;

    // output
    std::cout << "Device name: " << data["deviceName"] << std::endl;
    std::cout << "OS,Version: " << os_version << std::endl;
    std::cout << "Channel,Url: " << ch_url << std::endl;

    return resp->Json(device);
}

int updateDeviceName(HttpRequest* req, HttpResponse* resp)
{
    std::string dev_name = req->GetString("deviceName");

    std::cout << "\nupdate Device Name operation...\n";
    std::cout << "deviceName: " << dev_name << "\n";
    writeFile(PATH_DEVICE_NAME, dev_name);

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}

int updateChannel(HttpRequest* req, HttpResponse* resp)
{
    hv::Json response;
    std::string str_ch = req->GetString("channel");
    std::string str_url = req->GetString("serverUrl");

    std::cout << "\nupdate channel operation...\n";
    std::cout << "channel: " << str_ch << "\n";
    std::cout << "serverUrl: " << str_url << "\n";

    if (req->GetString("channel").empty()) {
        response["code"] = 1109;
        response["msg"] = "value error";
        response["data"] = hv::Json({});

        return resp->Json(response);
    }

    writeFile(PATH_UPGRADE_URL, str_ch + "," + str_url);

    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}

int setPower(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nset Power operation...\n";
    std::cout << "mode: " << req->GetString("mode") << "\n";

    int mode = stoi(req->GetString("mode"));

    if (mode) {
        printf("start to reboot system\n");
        system("reboot");
    } else {
        printf("start to shut down system\n");
        system("poweroff");
    }

    hv::Json response;

    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}

/* upgrade */
int updateSystem(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nstart to update System now...\n";

    // TODO

    g_progress = 0;

    hv::Json response;

    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}

int getUpdateProgress(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nget Update Progress...\n";

    // TODO

    hv::Json response;

    response["code"] = 0;
    response["msg"] = "";

    hv::Json data;
    data["progress"] = g_progress;
    if (g_progress < 100)
        g_progress += 10;
    response["data"] = data;

    return resp->Json(response);
}

int cancelUpdate(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\ncancel update...\n";

    // TODO
    g_progress = 0;

    hv::Json response;

    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}
