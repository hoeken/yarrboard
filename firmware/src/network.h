/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_NETWORK_H
#define YARR_NETWORK_H

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "config.h"

extern IPAddress apIP;
extern char wifi_ssid[YB_WIFI_SSID_LENGTH];
extern char wifi_pass[YB_WIFI_PASSWORD_LENGTH];
extern char wifi_mode[16];
extern char local_hostname[YB_HOSTNAME_LENGTH];
extern char uuid[13];
extern bool is_first_boot;

void network_setup();
void network_loop();

void setupWifi();
bool connectToWifi(const char* ssid, const char* pass);

#endif /* !YARR_NETWORK_H */