#ifndef __APP_IPC_SRV_H__
#define __APP_IPC_SRV_H__

#include "global_cfg.h"

EXTERN_C_START()

int app_ipc_Httpd_Init();
int app_ipc_Httpd_DeInit();

int app_WiFi_Init();
int app_WiFi_Deinit();

EXTERN_C_END()

#endif
