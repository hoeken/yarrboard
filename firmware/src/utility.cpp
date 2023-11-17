/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "utility.h"

double round2(double value) {
  return (long)(value * 100 + 0.5) / 100.0;
}

double round3(double value) {
  return (long)(value * 1000 + 0.5) / 1000.0;
}

double round4(double value) {
  return (long)(value * 10000 + 0.5) / 10000.0;
}

//variables for our framerate
int tickindex = 0;
int ticklist[YB_FPS_SAMPLES];
double ticksum = 0;
double framerate = 0;

// newtick is milliseconds since last frame
// returns average ticks per frame over the MAXSAMPLES last frames
// average will ramp up until the buffer is full
// from: https://stackoverflow.com/questions/87304/calculating-frames-per-second-in-a-game
double calculateFramerate(int newtick)
{
  ticksum -= ticklist[tickindex];  /* subtract value falling off */
  ticksum += newtick;              /* add new value */
  ticklist[tickindex] = newtick;   /* save new value so it can be subtracted later */
  tickindex = (tickindex + 1) % YB_FPS_SAMPLES; /* inc buffer index */

  /* return average */
  return((ticksum/YB_FPS_SAMPLES)*1000);
}