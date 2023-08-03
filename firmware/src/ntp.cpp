/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "ntp.h"
#include "time.h"
#include "sntp.h"

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
struct tm timeinfo;

void ntp_setup()
{
    //Setup our NTP to get the current time.
    sntp_set_time_sync_notification_cb(timeAvailable);
    sntp_servermode_dhcp(1);  // (optional)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
}

void ntp_loop()
{

}

// Callback function (get's called when time adjusts via NTP)
void timeAvailable(struct timeval *t) {
  Serial.print("NTP update: ");
  printLocalTime();
}

void printLocalTime()
{
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char buffer[40];
  strftime(buffer, 40, "%FT%T%z", &timeinfo);
  Serial.println(buffer);
}
