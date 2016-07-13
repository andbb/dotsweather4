#include <pebble.h>

#include "windows/main_window.h"

static void tick_handler(struct tm *time_now, TimeUnits changed) {
  main_window_update(time_now->tm_hour, time_now->tm_min);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick");
  
//  request_weather();
}

static void init() {
  main_window_push();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
//  tick_timer_service_subscribe(10*SECOND_UNIT, tick_handler);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Load");
}

static void deinit() {
  tick_timer_service_unsubscribe();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Unload");
}

int main() {
  init();
  app_event_loop();
  deinit();
}
