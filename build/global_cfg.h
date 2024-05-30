#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
#define EXTERN_C_START() extern "C" {
#define EXTERN_C_END() }
#else
#define EXTERN_C_START()
#define EXTERN_C_END()
#endif

/* HTTPD */
#define HTTPD_PORT 80
#define WWW(file) ("/mnt/dist/" file)
#define REDIRECT_URL "http://192.168.16.1/index.html"
#define DEFAULT_UPGRADE_URL "https://github.com/Seeed-Studio/reCamera"

/* WebSocket */
#define WS_PORT 8000

#define PATH_MODEL "/proc/device-tree/model"
#define PATH_ISSUE "/etc/issue"
#define PATH_DEVICE_NAME "/etc/device-name"
#define PATH_UPGRADE_URL "/etc/upgrade"

/* usertool.sh*/
#define SCRIPT_USER(action) ("/mnt/usertool.sh" \
                             " " #action " ")
#define SCRIPT_USER_NAME SCRIPT_USER(name)
#define SCRIPT_USER_PWD SCRIPT_USER(passwd)
#define SCRIPT_USER_SSH SCRIPT_USER(query_key)

/* wifitool.sh*/
#define SCRIPT_WIFI(action) ("/mnt/wifitool.sh" \
                             " " #action " ")
#define SCRIPT_WIFI_START SCRIPT_WIFI(start)
#define SCRIPT_WIFI_SCAN SCRIPT_WIFI(scan)
#define SCRIPT_WIFI_LIST SCRIPT_WIFI(list)
#define SCRIPT_WIFI_CONNECT SCRIPT_WIFI(connect)
#define SCRIPT_WIFI_SELECT SCRIPT_WIFI(select)
#define SCRIPT_WIFI_DISCONNECT SCRIPT_WIFI(disconnect)
#define SCRIPT_WIFI_STATUS SCRIPT_WIFI(status)
#define SCRIPT_WIFI_REMOVE SCRIPT_WIFI(remove)
#define SCRIPT_WIFI_STATE SCRIPT_WIFI(state)

#endif
