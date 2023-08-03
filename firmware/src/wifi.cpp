/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "wifi.h"
#include "prefs.h"
#include "server.h"

//for making a captive portal
// The default android DNS
IPAddress apIP(8,8,4,4);
const byte DNS_PORT = 53;
DNSServer dnsServer;

//default config info for our wifi
String wifi_ssid = "Yarrboard";
String wifi_pass = "";
String wifi_mode = "ap";
String local_hostname = "yarrboard";

//identify yourself!
String uuid;
bool is_first_boot = true;

void wifi_setup()
{
  if (preferences.isKey("local_hostname"))
    local_hostname = preferences.getString("local_hostname");

  //wifi login info.
  if (preferences.isKey("wifi_mode"))
  {
    is_first_boot = false;
    wifi_mode = preferences.getString("wifi_mode");
    wifi_ssid = preferences.getString("wifi_ssid");
    wifi_pass = preferences.getString("wifi_pass");
  }
  else
    is_first_boot = true;

  //get a unique ID for us
  byte mac[6];
  WiFi.macAddress(mac);
  uuid = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  Serial.print("UUID: ");
  Serial.println(uuid);

  //get an IP address
  setupWifi();
}

void wifi_loop()
{
  //run our dns... for AP mode
  if (wifi_mode.equals("ap"))
      dnsServer.processNextRequest();
}

void setupWifi()
{
  //some global config
  WiFi.setSleep(false);
  WiFi.setHostname(local_hostname.c_str());
  WiFi.useStaticBuffers(true);  //from: https://github.com/espressif/arduino-esp32/issues/7183
  WiFi.mode(WIFI_AP_STA);

  Serial.print("Hostname: ");
  Serial.print(local_hostname);
  Serial.println(".local");

  //which mode do we want?
  if (wifi_mode.equals("client"))
  {
    Serial.print("Client mode: ");
    Serial.print(wifi_ssid);
    Serial.print(" / ");
    Serial.println(wifi_pass);

    //try and connect
    connectToWifi(wifi_ssid, wifi_pass);
  }
  //default to AP mode.
  else
  {
    Serial.print("AP mode: ");
    Serial.print(wifi_ssid);
    Serial.print(" / ");
    Serial.println(wifi_pass);

    WiFi.softAP(wifi_ssid, wifi_pass);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    Serial.print("AP IP address: ");
    Serial.println(apIP);

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);
  }

  //setup our local name.
  if (!MDNS.begin(local_hostname)) {
    Serial.println("Error starting mDNS");
    return;
  }
  MDNS.addService("http", "tcp", 80);
}

bool connectToWifi(String ssid, String pass)
{
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());

  // How long to try for?
  int tryDelay = 500;
  int numberOfTries = 60;

  // Wait for the WiFi event
  while (true)
  {
    switch (WiFi.status())
    {
      case WL_NO_SSID_AVAIL:
        Serial.println("[WiFi] SSID not found");
        return false;
        break;
      case WL_CONNECT_FAILED:
        Serial.print("[WiFi] Failed");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("[WiFi] Connection was lost");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("[WiFi] Scan is completed");
        break;
      case WL_DISCONNECTED:
        Serial.println("[WiFi] WiFi is disconnected");
        break;
      case WL_CONNECTED:
        Serial.println("[WiFi] WiFi is connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());
        return true;
        break;
      default:
        Serial.print("[WiFi] WiFi Status: ");
        Serial.println(WiFi.status());
        break;
    }

    //have we hit our limit?
    if(numberOfTries <= 0)
    {
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return false;
    } else {
      numberOfTries--;
    }

    delay(tryDelay);
  }

  return false;
}