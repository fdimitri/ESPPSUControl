#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#else
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#endif

#include <stdio.h>

#include "structs.h"
#include "pmbus.h"
#include "display.h"

#ifndef ARDUINO
#include "linux_arduino_wrapper.h"
#endif

void stats_update_item(statsItem *cItem, float f);
void stats_collect(int pfd);
float stats_get_average(statsItem *cItem);
void stats_initialize();

statsRecorder stats;

void stats_collect(int pfd) {
  uint16_t buf, vomode;
  struct statsItem *cItem;
  float f;

  pmbus_request_by_name(pfd, "VOUT_MODE", (byte *) &buf);
  vomode = buf;

  cItem = &stats.inAmps;
  pmbus_request_by_name(pfd, "READ_IIN", (byte *) &buf);
  f = pmbus_convert_linear11_to_float(buf);
  stats_update_item(cItem, f);

  cItem = &stats.inVolts;
  pmbus_request_by_name(pfd, "READ_VIN", (byte *) &buf);
  f = pmbus_convert_linear11_to_float(buf);
  stats_update_item(cItem, f);

  cItem = &stats.outVolts;
  pmbus_request_by_name(pfd, "READ_VOUT", (byte *) &buf);
  f = pmbus_convert_linear16_to_float(buf, vomode);
  stats_update_item(cItem, f);

  cItem = &stats.outAmps;
  pmbus_request_by_name(pfd, "READ_IOUT", (byte *) &buf);
  f = pmbus_convert_linear11_to_float(buf);
  stats_update_item(cItem, f);

  return;
}

void stats_update_item(statsItem *cItem, float f) {
  if (cItem->peak < f) cItem->peak = f;
  if (cItem->min > f) cItem->min = f;
  cItem->samples[cItem->tptr++] = f;
  return;  
}

float stats_get_average(statsItem *cItem) {
  float t;
  for (int i = 0; i < STATS_NSAMPLES; i++) t += cItem->samples[i];
  return(t / STATS_NSAMPLES);
}

void stats_initialize() {
  memset((void *) &stats, 0, sizeof(stats));

}
