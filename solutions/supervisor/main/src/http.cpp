#include <iostream>
#include <string>

#include "hv/HttpServer.h"
#include "hv/hthread.h"    // import hv_gettid
#include "hv/hasync.h"     // import hv::async

#include "http.h"
#include "utils_device.h"
#include "utils_user.h"
#include "utils_wifi.h"

using namespace hv;

extern "C" {

static HttpServer server;

static void register_Httpd_Redirect(HttpService& router)
{
    router.GET("/hotspot-detect*", [](HttpRequest* req, HttpResponse* resp) { // IOS
        // resp->File(WWW("err.html"));
        std::cout << "\n[/hotspot-detect*]current url: " << req->Url() << "\n";
        std::cout << "-> redirect to " << REDIRECT_URL << "\n";

        return resp->Redirect(REDIRECT_URL);
    });

    router.GET("/generate*", [](HttpRequest* req, HttpResponse* resp) { // android
        // resp->File(WWW("err.html"));
        std::cout << "\n[/generate*]current url: " << req->Url() << "\n";
        std::cout << "-> redirect to " << REDIRECT_URL << "\n";

        return resp->Redirect(REDIRECT_URL);
    });

    router.GET("/*.txt", [](HttpRequest* req, HttpResponse* resp) { // windows
        // resp->File(WWW("err.html"));
        std::cout << "\n[/*.txt]current url: " << req->Url() << "\n";
        std::cout << "-> redirect to " << REDIRECT_URL << "\n";

        return resp->Redirect(REDIRECT_URL);
    });

    router.GET("/index.html", [](HttpRequest* req, HttpResponse* resp) {
        return resp->File(WWW("index.html"));
    });
}

static void register_User_Api(HttpService& router)
{
    router.GET("/api/userMgr/queryUserInfo", queryUserInfo);
    router.POST("/api/userMgr/updateUserName", updateUserName);
    router.POST("/api/userMgr/updatePassword", updatePassword);
    router.POST("/api/userMgr/addSShkey", addSShkey);
    router.POST("/api/userMgr/deleteSShkey", deleteSShkey);
}

static void register_WiFi_Api(HttpService& router)
{
    router.GET("/api/wifiMgr/queryWiFiInfo", queryWiFiInfo);
    router.POST("/api/wifiMgr/scanWiFi", scanWiFi);
    router.POST("/api/wifiMgr/connectWiFi", connectWiFi);
    router.POST("/api/wifiMgr/disconnectWiFi", disconnectWiFi);
    router.POST("/api/wifiMgr/switchWiFi", switchWiFi);
    router.GET("/api/wifiMgr/getWifiStatus", getWifiStatus);
    router.POST("/api/wifiMgr/autoConnectWiFi", autoConnectWiFi);
    router.POST("/api/wifiMgr/forgetWiFi", forgetWiFi);
}

static void register_Device_Api(HttpService& router)
{
    router.POST("/api/deviceMgr/getSystemUpdateVesionInfo", getSystemUpdateVesionInfo);
    router.GET("/api/deviceMgr/queryDeviceInfo", queryDeviceInfo);
    router.POST("/api/deviceMgr/updateDeviceName", updateDeviceName);
    router.POST("/api/deviceMgr/updateChannel", updateChannel);
    router.POST("/api/deviceMgr/setPower", setPower);
    router.POST("/api/deviceMgr/updateSystem", updateSystem);
    router.GET("/api/deviceMgr/getUpdateProgress", getUpdateProgress);
    router.POST("/api/deviceMgr/cancelUpdate", cancelUpdate);
}

static void register_WebSocket(HttpService& router)
{
    router.GET("/api/deviceMgr/getCameraWebsocketUrl", [](HttpRequest* req, HttpResponse* resp) {
        hv::Json data;
        data["websocketUrl"] = "ws://" + req->host + ":" + std::to_string(WS_PORT);
        hv::Json res;
        res["code"] = 0;
        res["msg"] = "";
        res["data"] = data;

        std::string s_time = req->GetParam("time");//req->GetString("time");
        int64_t time = std::stoll(s_time);
        time /= 1000; // to sec
        std::string cmd = "date -s @" + std::to_string(time);
        system(cmd.c_str());

        std::cout << "WebSocket:" << data["websocketUrl"] << "\n";
        return resp->Json(res);
    });
}

int app_ipc_Httpd_Init()
{
    static HttpService router;

    /* Static file service */
    // curl -v http://ip:port/
    router.Static("/", WWW(""));

    register_Httpd_Redirect(router);
    register_User_Api(router);
    register_WiFi_Api(router);
    register_Device_Api(router);
    register_WebSocket(router);

    server.service = &router;
    server.port = HTTPD_PORT;
    server.start();

    return 0;
}

int app_ipc_Httpd_DeInit()
{
    server.stop();
    hv::async::cleanup();
    return 0;
}

} // extern "C" {