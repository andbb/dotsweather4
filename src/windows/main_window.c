#include "main_window.h"

static Window *s_window;
static Layer *s_canvas;

static int s_hours, s_minutes;
static TextLayer *s_temperature_layer;
static TextLayer *s_city_layer;
//static RotBitmapLayer *s_icon_layer;
//static RotBitmapLayer *s_wind_layer;
static BitmapLayer *s_icon_layer;
static BitmapLayer *s_wind_layer;
static GBitmap *s_icon_bitmap = NULL;
static GBitmap *s_wind_bitmap = NULL;
static int t10=0;
static float temperature=0;
static int winddeg=360;
static int w100=360;
static float wind=0;
static int dirdeg=0;
static int speed100=360;
static double speed=0;
static AppSync s_sync;
static uint8_t s_sync_buffer[64];

enum WeatherKey {
     WEATHER_TEMPERATURE_KEY= 0x0, // TUPLE_INT
     WIND_DIR_KEY= 0x1,            // TUPLE_INT
     WIND_SPEED_KEY= 0x2,          // TUPLE_INT
     DIR_KEY= 0x3,                 // TUPLE_INT
     SPEED_DIR_KEY= 0x4            // TUPLE_INT
  };
/*
enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
};
*/

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_WIND, // 0
  RESOURCE_ID_IMAGE_DIRECTION  , // 1
};

static const uint32_t WIND_ICONS[] = {
  RESOURCE_ID_IMAGE_WIND_N, // 0
  RESOURCE_ID_IMAGE_WIND_NE, // 1
  RESOURCE_ID_IMAGE_WIND_E, // 2
  RESOURCE_ID_IMAGE_WIND_SE, // 3
  RESOURCE_ID_IMAGE_WIND_S, // 4
  RESOURCE_ID_IMAGE_WIND_SW, // 5
  RESOURCE_ID_IMAGE_WIND_W, // 6
  RESOURCE_ID_IMAGE_WIND_NW, // 7
};

static const uint32_t DIRECTION_ICONS[] = {
  RESOURCE_ID_IMAGE_DIR_N, // 0
  RESOURCE_ID_IMAGE_DIR_NE, // 1
  RESOURCE_ID_IMAGE_DIR_E, // 2
  RESOURCE_ID_IMAGE_DIR_SE, // 3
  RESOURCE_ID_IMAGE_DIR_S, // 4
  RESOURCE_ID_IMAGE_DIR_SW, // 5
  RESOURCE_ID_IMAGE_DIR_W, // 6
  RESOURCE_ID_IMAGE_DIR_NW, // 7
};

int deg_to_brng(float dirdeg){
  int brng;
  brng=((dirdeg+22.5)/45);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Icon bearing %i", brng);
  return brng;
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

int headdir(int dirdeg) {
  int windhead;
  /*char windhead[];
  if (dirdeg<22) {
          windhead='N';
        } else
        if (dirdeg<67) {
          windhead='NE';
        } else
        if (dirdeg<112) {
          windhead='E';
        } else
        if (dirdeg<157) {
          windhead='SE';
        } else
        if (dirdeg<202) {
          windhead='S';
        } else
        if (dirdeg<247) {
          windhead='SW';
        } else
        if (dirdeg<292) {
          windhead='W';
        } else
        if (dirdeg<337) {
          windhead='NW';
        } else
        if (dirdeg<361) {
          windhead='N';
        }
*/
  windhead=((dirdeg+22)%16);
  return windhead;
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  char citytxt[]="Holte";
   char ttxt[]="Varmt";
  static int i = 42;

/* The string/char-buffer to hold the string representation of int.
 * Assuming a 4byte int, this needs to be a maximum of upto 12bytes.
 * to hold the number, optional negative sign and the NUL-terminator.
 */
  char buf[] = "00000000000";    /* <-- implicit NUL-terminator at the end here */
  char buf2[] = "00000000000";    /* <-- implicit NUL-terminator at the end here */
//  APP_LOG(APP_LOG_LEVEL_DEBUG, "Key %lu", (unsigned long) key);
   text_layer_set_text(s_temperature_layer, buf2);

  switch (key) {
    case WEATHER_TEMPERATURE_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "WTK ");
      t10=new_tuple->value->int16;
      temperature=t10/10;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "WTK %i",t10);
      break;
    case WIND_DIR_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "WDK ");
      winddeg=new_tuple->value->int16;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "WDeg %i", winddeg);
      snprintf(buf, sizeof(buf), "Wind %i", winddeg);
//      text_layer_set_text(s_city_layer, buf);
      break;
    case WIND_SPEED_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "WSK ");
      w100=new_tuple->value->int16;
      wind=w100/100;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "WSK %i",w100);
      break;
    case DIR_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "DirK ");
      dirdeg=new_tuple->value->int16;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "DirK %i",dirdeg);
      break;
    case SPEED_DIR_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "SDK ");
      speed100=new_tuple->value->int16;
      speed=speed100/100;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Speed %i",speed100);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Speed %d",speed100/100);
    
//      snprintf(buf2, sizeof(buf2), "Speed %f", speed);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Speedstr %s",buf2);
//      text_layer_set_text(s_temperature_layer, buf2);
//      text_layer_set_text(s_temperature_layer, ttxt);
//      text_layer_set_text(s_city_layer, citytxt);
      break;
  }
}

static void request_weather(void) {
//void request_weather(void) {
  DictionaryIterator *iter;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RequestWW");

  AppMessageResult result =app_message_outbox_begin(&iter);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "AMOB ");
  //APP_LOG(APP_LOG_LEVEL_DEBUG,  iter);
  
  
  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "I_err");
  
    // Error creating outbound message
    return;
  }

  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int), true);
  dict_write_end(iter);

  app_message_outbox_send();
}
/*
static void weather_window(Window *window) {
//static void weather_window(void) {
//    static char s_buffer[8];
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_icon_layer = bitmap_layer_create(GRect(0, 10, bounds.size.w, 80));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));

  s_temperature_layer = text_layer_create(GRect(0, 90, bounds.size.w, 32));
  text_layer_set_text_color(s_temperature_layer, GColorWhite);
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));

  s_city_layer = text_layer_create(GRect(0, 122, bounds.size.w, 32));
  text_layer_set_text_color(s_city_layer, GColorWhite);
  text_layer_set_background_color(s_city_layer, GColorClear);
  text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_city_layer));

  Tuplet initial_values[] = {
    TupletInteger(WEATHER_TEMPERATURE_KEY, (int16_t) 1),
    TupletInteger(WIND_DIR_KEY, (int16_t) 1),
    TupletInteger(WIND_SPEED_KEY, (int16_t) 1),
    TupletInteger(DIR_KEY, (int16_t) 1),
    TupletInteger(SPEED_DIR_KEY, (int16_t) 1)
        };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  request_weather();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Request");
 
//  strftime(s_buffer, sizeof(s_buffer), "%H:%M", tick_time);
//  APP_LOG(APP_LOG_LEVEL_INFO, "WW RW Time is now %s", s_buffer);
//  APP_LOG(APP_LOG_LEVEL_INFO, "WW RW Time ");
 
}
*/
static int32_t get_angle_for_hour(int hour) {
  // Progress through 12 hours, out of 360 degrees
  return (hour * 360) / 12;
}

static int32_t get_angle_for_minute(int minute) {
  // Progress through 60 minutes, out of 360 degrees
  return (minute * 360) / 60;
}

static void layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // 12 hours only, with a minimum size
  s_hours -= (s_hours > 12) ? 12 : 0;

  // Minutes are expanding circle arc
  int minute_angle = get_angle_for_minute(s_minutes);
  GRect frame = grect_inset(bounds, GEdgeInsets(4 * INSET));
  graphics_context_set_fill_color(ctx, MINUTES_COLOR);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 20, 0, DEG_TO_TRIGANGLE(minute_angle));

  // Adjust geometry variables for inner ring
  frame = grect_inset(frame, GEdgeInsets(3 * HOURS_RADIUS));

  // Hours are dots
  for(int i = 0; i < 12; i++) {
    int hour_angle = get_angle_for_hour(i);
    GPoint pos = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(hour_angle));

    graphics_context_set_fill_color(ctx, i <= s_hours ? HOURS_COLOR : HOURS_COLOR_INACTIVE);
    graphics_fill_circle(ctx, pos, HOURS_RADIUS);
  }
//  request_weather();
//  weather_window();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  char buf[] = "Start";    /* <-- implicit NUL-terminator at the end here */
  
//   request_weather();
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas = layer_create(bounds);

  layer_set_update_proc(s_canvas, layer_update_proc);
  layer_add_child(window_layer, s_canvas);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Define rotation ");


//  s_icon_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[0]);
//  s_icon_layer = rot_bitmap_layer_create(s_icon_bitmap);

  s_wind_layer = bitmap_layer_create(GRect(0, 30, bounds.size.w, 100));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_wind_layer));

  s_icon_layer = bitmap_layer_create(GRect(0, 30, bounds.size.w, 100));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));

//  layer_add_child(window_layer,  bitmap_layer_get_layer((BitmapLayer *)s_icon_layer));
        
//  layer_add_child(window_layer, rot_bitmap_layer_get_layer(s_icon_layer));

//  s_wind_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[1]);
//  s_wind_layer = rot_bitmap_layer_create(s_wind_bitmap);
  
//  layer_add_child(window_layer,  bitmap_layer_get_layer((BitmapLayer *)s_wind_layer));

//  layer_add_child(window_layer, bitmap_layer_get_layer(s_wind_layer));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Rotation defined ");

  s_temperature_layer = text_layer_create(GRect(0, 132, bounds.size.w, 32));
  text_layer_set_text_color(s_temperature_layer, GColorWhite);
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));

  s_city_layer = text_layer_create(GRect(0, 100, bounds.size.w, 32));
  text_layer_set_text_color(s_city_layer, GColorWhite);
  text_layer_set_background_color(s_city_layer, GColorClear);
  text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
  text_layer_set_text(s_city_layer, buf);
  layer_add_child(window_layer, text_layer_get_layer(s_city_layer));
  
  /*
  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 1),
    TupletCString(WEATHER_TEMPERATURE_KEY, "1234\u00B0C"),
    TupletCString(WEATHER_CITY_KEY, "St Pebblesburg"),
  };
  */
    Tuplet initial_values[] = {
//    TupletInteger(WEATHER_ICON_KEY, (int16_t) 1),
    TupletInteger(WEATHER_TEMPERATURE_KEY, (int16_t) 1),
    TupletInteger(WIND_DIR_KEY, (int16_t) 1),
    TupletInteger(WIND_SPEED_KEY, (int16_t) 1),
    TupletInteger(DIR_KEY, (int16_t) 1),
    TupletInteger(SPEED_DIR_KEY, (int16_t) 1)
        };

   app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "WLoad");

 
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  window_destroy(s_window);
}

void main_window_push() {
  s_window = window_create();
  window_set_background_color(s_window, BG_COLOR);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  app_message_open(64, 64);
}

void main_window_update(int hours, int minutes) {
/*   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);

  if (!iter) {
    // Error creating outbound message
    return;
  }

  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int), true);
  dict_write_end(iter);

  app_message_outbox_send();
*/  
  request_weather();
  s_hours = hours;
  s_minutes = minutes;
//  layer_mark_dirty(s_canvas);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MWupd");
}
