#include <iostream>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#include "http.h"
#include "daemon.h"

static void exitHandle(int signo) {
    stopDaemon();
    system(SCRIPT_WIFI_STOP);
    deinitHttpd();
    closelog();

    exit(0);
}

static void initSupervisor() {
    initWiFi();
    initHttpd();
    initDaemon();

    signal(SIGINT, &exitHandle);
    signal(SIGTERM, &exitHandle);

    openlog("supervisor", LOG_CONS | LOG_PID, 0);
}

int main(int argc, char** argv) {
    initSupervisor();

    while(1) sleep(1000);

    system(SCRIPT_WIFI_STOP);
    deinitHttpd();
    closelog();

    return 0;
}