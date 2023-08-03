#ifndef YARR_WIFI_H
#define YARR_WIFI_H

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

extern IPAddress apIP;
extern String wifi_ssid;
extern String wifi_pass;
extern String wifi_mode;
extern String local_hostname;
extern String uuid;
extern bool is_first_boot;

void wifi_setup();
void wifi_loop();

void setupWifi();
bool connectToWifi(String ssid, String pass);

#endif /* !YARR_WIFI_H */