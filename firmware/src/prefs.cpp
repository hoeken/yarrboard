/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "prefs.h"

Preferences preferences;

void prefs_setup()
{
    preferences.begin("yarrboard", false);
}