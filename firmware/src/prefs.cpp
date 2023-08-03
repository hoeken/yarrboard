#include "prefs.h"

Preferences preferences;

void prefs_setup()
{
    preferences.begin("yarrboard", false);
}