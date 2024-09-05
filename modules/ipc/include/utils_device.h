#ifndef _APP_IPC_DEVICE_H_
#define _APP_IPC_DEVICE_H_

std::string readFile(const std::string& path, const std::string& defaultname = "EmptyContent");

int getSystemUpdateVesionInfo(HttpRequest* req, HttpResponse* resp);

int queryDeviceInfo(HttpRequest* req, HttpResponse* resp);
int updateDeviceName(HttpRequest* req, HttpResponse* resp);

int updateChannel(HttpRequest* req, HttpResponse* resp);
int setPower(HttpRequest* req, HttpResponse* resp);
int updateSystem(HttpRequest* req, HttpResponse* resp);

int getUpdateProgress(HttpRequest* req, HttpResponse* resp);
int cancelUpdate(HttpRequest* req, HttpResponse* resp);

int getDeviceList(HttpRequest* req, HttpResponse* resp);
int getDeviceInfo(HttpRequest* req, HttpResponse* resp);
int getDeviceIP(HttpRequest* req, HttpResponse* resp);

#endif
