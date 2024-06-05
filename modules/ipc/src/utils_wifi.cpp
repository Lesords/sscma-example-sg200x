#include <iostream>
#include <map>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "HttpServer.h"
#include "global_cfg.h"
#include "utils_wifi.h"

typedef struct _WIFI_INFO_S {
    int id;
    int connectedStatus;
    int autoConnect;
} WIFI_INFO_S;

std::vector<std::vector<std::string>> g_wifiList;
std::map<std::string, WIFI_INFO_S> g_wifiInfo;
std::string g_currentWifi;
int g_wifiMode = 0;

static int getWifiInfo(std::vector<std::string>& wifiStatus)
{
    FILE* fp;
    char info[128];

    fp = popen(SCRIPT_WIFI_STATUS, "r");
    if (fp == NULL) {
        printf("Failed to run `%s`\n", SCRIPT_WIFI_STATUS);
        return -1;
    }

    while (fgets(info, sizeof(info) - 1, fp) != NULL) {
        std::string s(info);
        if (s.back() == '\n') {
            s.erase(s.size() - 1);
        }
        wifiStatus.push_back(s);
    }

    pclose(fp);

    return 0;
}

static int getWifiList()
{
    FILE* fp;
    char info[128];
    std::vector<std::vector<std::string>> wifiList;

    fp = popen(SCRIPT_WIFI_SCAN, "r");
    if (fp == NULL) {
        printf("Failed to run `%s`\n", SCRIPT_WIFI_SCAN);
        return -1;
    }

    while (fgets(info, sizeof(info) - 1, fp) != NULL) {
        std::vector<std::string> wifi;

        char* token = strtok(info, " ");
        while (token != NULL) {
            wifi.push_back(std::string(token));
            token = strtok(NULL, " ");
        }

        if (!wifi.empty()) {
            // TODO:: Need to determine if wifi is encrypted or not
            wifi[2] = "1";
            wifiList.push_back(wifi);
        }
    }

    if (!wifiList.empty()) {
        g_wifiList = wifiList;
    }

    pclose(fp);
    return 0;
}

static int updateConnectedWifiInfo()
{
    FILE* fp;
    char info[128];

    std::cout << "updateConnectedWifiInfo operation\n";

    fp = popen(SCRIPT_WIFI_LIST, "r");
    if (fp == NULL) {
        printf("Failed to run `%s`\n", SCRIPT_WIFI_LIST);
        return -1;
    }

    while (fgets(info, sizeof(info) - 1, fp) != NULL) {
        std::vector<std::string> wifi;

        char* token = strtok(info, " ");
        while (token != NULL) {
            wifi.push_back(std::string(token));
            token = strtok(NULL, " ");
        }

        g_wifiInfo[wifi[1]].id = stoi(wifi[0]);
        g_wifiInfo[wifi[1]].connectedStatus = 1;
        g_wifiInfo[wifi[1]].autoConnect = 1;
    }

    pclose(fp);

    return 0;
}

static int getLocalNetInfo(const char *name, std::string &ip, std::string &mask, std::string &mac)
{
    int sock;
    struct ifreq ifr;
    char info[INET_ADDRSTRLEN];
    char mac_address[18];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFADDR");
        close(sock);
        return 1;
    }
    inet_ntop(AF_INET, &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr, info, INET_ADDRSTRLEN);
    ip = info;

    if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0) {
        perror("ioctl SIOCGIFNETMASK");
        close(sock);
        return 1;
    }
    inet_ntop(AF_INET, &((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr, info, INET_ADDRSTRLEN);
    mask = info;

    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFHWADDR");
        close(sock);
        exit(1);
    }
    sprintf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)ifr.ifr_hwaddr.sa_data[0],
            (unsigned char)ifr.ifr_hwaddr.sa_data[1],
            (unsigned char)ifr.ifr_hwaddr.sa_data[2],
            (unsigned char)ifr.ifr_hwaddr.sa_data[3],
            (unsigned char)ifr.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    mac = mac_address;

    close(sock);

    std::cout << "ip: " << ip << "\n";
    std::cout << "mask: " << mask << "\n";
    std::cout << "mac: " << mac << "\n";

	return 0;
}

int queryWiFiInfo(HttpRequest* req, HttpResponse* resp)
{
    std::vector<std::string> wifiStatus;

    if (getWifiInfo(wifiStatus) != 0) {
        return -1;
    }

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";

    hv::Json data;
    data["status"] = 3;

    hv::Json wifiInfo;

    wifiInfo["ssid"] = wifiStatus[0];
    g_currentWifi = wifiStatus[0];
    if (wifiStatus[1] != "") {
        wifiInfo["auth"] = 1;
    } else {
        wifiInfo["auth"] = 0;
    }
    wifiInfo["signal"] = 0;
    wifiInfo["connectedStatus"] = 1;
    wifiInfo["macAddres"] = wifiStatus[2];
    wifiInfo["ip"] = wifiStatus[3];
    wifiInfo["ipAssignment"] = 0;
    wifiInfo["subnetMask"] = "255.255.255.0";
    wifiInfo["autoConnect"] = 0;

    data["wifiInfo"] = wifiInfo;
    response["data"] = data;

    return resp->Json(response);
}

int scanWiFi(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nscan WiFi operation...\n";
    std::cout << "scanTime: " << req->GetString("scanTime") << "\n";

    std::string ip, mask, mac;

    if (updateConnectedWifiInfo() != 0) {
        return -1;
    }

    if (getWifiList() != 0) {
        return -1;
    }

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";

    hv::Json data;
    hv::Json etherInfo;
    hv::Json wifiInfo;
    std::vector<hv::Json> wifiInfoList;

    std::cout << "current wifi: =" << g_currentWifi << "=\n";

    if (getLocalNetInfo("wlan0", ip, mask, mac) == 0) {
        std::cout << "wlan0 connect here\n";
    }

    for (auto wifi : g_wifiList) {
        wifiInfo.clear();
        wifiInfo["ssid"] = wifi[0];
        wifiInfo["auth"] = stoi(wifi[2]);
        wifiInfo["signal"] = stoi(wifi[1]);

        if (g_wifiInfo.find(wifi[0]) != g_wifiInfo.end()) {
            wifiInfo["connectedStatus"] = g_wifiInfo[wifi[0]].connectedStatus;
        } else {
            wifiInfo["connectedStatus"] = 0;
        }

        wifiInfo["macAddress"] = wifi[3].substr(0, 17);

        if (wifi[0].compare(g_currentWifi) == 0) {
            wifiInfo["ip"] = ip;
            wifiInfo["ipAssignment"] = 0;
            wifiInfo["subnetMask"] = mask;
            wifiInfo["dns1"] = "-";
            wifiInfo["dns2"] = "-";
            wifiInfo["autoConnect"] = 0;
        } else {
            wifiInfo["ip"] = "";
            wifiInfo["ipAssignment"] = 0;
            wifiInfo["subnetMask"] = "-";
            wifiInfo["dns1"] = "-";
            wifiInfo["dns2"] = "-";
            wifiInfo["autoConnect"] = 0;
        }
        wifiInfoList.push_back(wifiInfo);
    }

    if (getLocalNetInfo("eth0", ip, mask, mac) == 0) {
        std::cout << "eth0 connect here\n";
    }

    if (ip.find("169.254") != std::string::npos) {
        etherInfo["connectedStatus"] = 0;
        etherInfo["macAddres"] = "-";
        etherInfo["ip"] = "-";
        etherInfo["ipAssignment"] = 0;
        etherInfo["subnetMask"] = "-";
        etherInfo["dns1"] = "-";
        etherInfo["dns2"] = "-";
    } else {
        etherInfo["connectedStatus"] = 1;
        etherInfo["macAddres"] = mac;
        etherInfo["ip"] = ip;
        etherInfo["ipAssignment"] = 0;
        etherInfo["subnetMask"] = mask;
        etherInfo["dns1"] = "-";
        etherInfo["dns2"] = "-";
    }

    data["etherinfo"] = etherInfo;
    data["wifiInfoList"] = hv::Json(wifiInfoList);
    response["data"] = data;

    // std::cout << "response: " << response.dump(2) << "\n";

    return resp->Json(response);
}

int connectWiFi(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nconnect WiFi...\n";
    std::cout << "ssid: " << req->GetString("ssid") << "\n";
    std::cout << "ssid: " << req->GetString("password") << "\n";

    std::string msg;
    int id;
    FILE* fp;
    char info[128];
    char cmd[128] = "";

    if (req->GetString("password").empty()) {
        strcpy(cmd, SCRIPT_WIFI_SELECT);
        id = g_wifiInfo[req->GetString("ssid")].id;
        strcat(cmd, std::to_string(id).c_str());
    } else {
        strcpy(cmd, SCRIPT_WIFI_CONNECT);
        strcat(cmd, req->GetString("ssid").c_str());
        strcat(cmd, " ");
        strcat(cmd, req->GetString("password").c_str());
    }

    printf("cmd: %s\n", cmd);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run %s\n", cmd);
        return -1;
    }

    if (fgets(info, sizeof(info) - 1, fp) != NULL) {
        id = stoi(std::string(info));
    }

    if (fgets(info, sizeof(info) - 1, fp) != NULL) {
        msg = std::string(info);
        if (msg.back() == '\n') {
            msg.erase(msg.size() - 1);
        }
    }

    hv::Json response;

    if (msg.compare("OK") != 0) {
        response["code"] = 1112;
        response["msg"] = msg;
    } else {
        response["code"] = 0;
        response["msg"] = "";
        g_wifiInfo[req->GetString("ssid")] = { id, 1, 1 };
    }
    response["data"] = hv::Json({});

    return resp->Json(response);
}

int disconnectWiFi(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\ndisconnect WiFi...\n";
    std::cout << "ssid: " << req->GetString("ssid") << "\n";

    std::string msg;
    int id = g_wifiInfo[req->GetString("ssid")].id;
    FILE* fp;
    char info[128];
    char cmd[128] = SCRIPT_WIFI_DISCONNECT;

    printf("id: %d\n", id);

    strcat(cmd, std::to_string(id).c_str());
    printf("cmd: %s\n", cmd);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run %s\n", cmd);
        return -1;
    }

    if (fgets(info, sizeof(info) - 1, fp) != NULL) {
        msg = std::string(info);
        if (msg.back() == '\n') {
            msg.erase(msg.size() - 1);
        }
    }

    hv::Json response;

    if (msg.compare("OK") != 0) {
        response["code"] = 1112;
        response["msg"] = msg;
    } else {
        response["code"] = 0;
        response["msg"] = "";
    }
    response["data"] = hv::Json({});

    return resp->Json(response);
}

int switchWiFi(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nswitch WiFi operation...\n";
    std::cout << "mode: " << req->GetString("mode") << "\n";

    g_wifiMode = stoi(req->GetString("mode"));

    std::cout << "g_wifiMode: " << g_wifiMode << "\n";

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}

int getWifiStatus(HttpRequest* req, HttpResponse* resp)
{
    FILE* fp;
    char info[128];
    hv::Json response, data;

    response["code"] = 0;
    response["msg"] = "";

    fp = popen(SCRIPT_WIFI_STATE, "r");
    if (fp == NULL) {
        printf("Failed to run `%s`\n", SCRIPT_WIFI_STATE);
        return -1;
    }

    if (fgets(info, sizeof(info) - 1, fp) != NULL) {
        std::string s(info);
        if (s.back() == '\n') {
            s.erase(s.size() - 1);
        }

        if (s.compare("COMPLETED") == 0) {
            data["status"] = 1;
        } else if (s.compare("INACTIVE") == 0) {
            data["status"] = 2;
        } else {
            data["status"] = 2;
        }
    }

    pclose(fp);
    response["data"] = data;

    return resp->Json(response);
}

int autoConnectWiFi(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nauto Connect operation...\n";
    std::cout << "ssid: " << req->GetString("ssid") << "\n";
    std::cout << "mode: " << req->GetString("mode") << "\n";

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}

int forgetWiFi(HttpRequest* req, HttpResponse* resp)
{
    std::cout << "\nforget WiFi operation...\n";
    std::cout << "ssid: " << req->GetString("ssid") << "\n";

    std::string msg;
    int id;
    FILE* fp;
    char info[128];
    char cmd[128] = SCRIPT_WIFI_REMOVE;

    id = g_wifiInfo[req->GetString("ssid")].id;
    strcat(cmd, std::to_string(id).c_str());

    printf("cmd: %s\n", cmd);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run %s\n", cmd);
        return -1;
    }

    if (fgets(info, sizeof(info) - 1, fp) != NULL) {
        msg = std::string(info);
        if (msg.back() == '\n') {
            msg.erase(msg.size() - 1);
        }
    }

    auto it = g_wifiInfo.find(req->GetString("ssid"));
    if (it != g_wifiInfo.end()) {
        g_wifiInfo.erase(it);
    }

    hv::Json response;
    response["code"] = 0;
    response["msg"] = "";
    response["data"] = hv::Json({});

    return resp->Json(response);
}
