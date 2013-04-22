#include <stdio.h>
#include <math.h>
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "xprintf.h"
#include "common.h"
#include "tinymt32.h"

// defines
#define MY_UUID { 0x74, 0x19, 0xF2, 0x5C, 0x82, 0x1B, 0x4B, 0xD8, 0x93, 0xD5, 0x98, 0x7E, 0x31, 0x15, 0xA8, 0xB6 }
PBL_APP_INFO(MY_UUID,
             "Fireflies", "Little Hiccup",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);
#define NUM_PARTICLES 100 
#define COOKIE_MY_TIMER 1
#define max(a,b) a > b ? a : b
#define min(a,b) ((a) > (b) ? (a) : (b))

// typedefs
typedef struct FParticle 
{
  GPoint position;
  GPoint grav_center;
  float dx;
  float dy;
  float power;
} FParticle;
#define FParticle(px, py, gx, gy, power) ((FParticle){{(px), (py)}, {(gx), (gy)}, 0.0F, 0.0F, power})

// globals
FParticle particles[NUM_PARTICLES];
Window window;
Layer square_layer;
TextLayer text_header_layer;
AppTimerHandle timer_handle;
tinymt32_t rndstate;

const GPathInfo SQUARE_POINTS = {
  4,
  (GPoint []) {
    {-25, -25},
    {-25,  25},
    { 25,  25},
    { 25, -25}
  }
};

GPath square_path;

#define MAX_SPEED      1
#define MAX_SPEED_NEG -1

int random_in_range(int min, int max) {
  return min + (int)(tinymt32_generate_float01(&rndstate) * ((max - min) + 1));
}

float random_in_rangef(float min, float max) {
  return min + (float)(tinymt32_generate_float01(&rndstate) * ((max - min)));
}

GPoint random_point_in_screen() {
  return GPoint(random_in_range(0, window.layer.frame.size.w+1), 
                random_in_range(0, window.layer.frame.size.h+1));
}

#define SCREEN_MARGIN 0.0F
GPoint random_point_roughly_in_screen() {
  return GPoint(random_in_range(-window.layer.frame.size.w*SCREEN_MARGIN, window.layer.frame.size.w+1+(window.layer.frame.size.w*SCREEN_MARGIN)), 
                random_in_range(-window.layer.frame.size.h*SCREEN_MARGIN, window.layer.frame.size.h+1+(window.layer.frame.size.h*SCREEN_MARGIN)));
}

#define JITTER 1.0F

void update_particle(int i) {
  // 
  // if(tinymt32_generate_float01(&rndstate) < 0.4F) {
  //   particles[i].dx += random_in_rangef(-JITTER, JITTER);
  //   particles[i].dy += random_in_rangef(-JITTER, JITTER);
  // }

  // gravitate towards goal
  particles[i].dx += -(particles[i].position.x - particles[i].grav_center.x)/particles[i].power;
  particles[i].dy += -(particles[i].position.y - particles[i].grav_center.y)/particles[i].power;


  // damping
  particles[i].dx *= 0.999F;
  particles[i].dy *= 0.999F;

  // snap to max
  if(particles[i].dx >  MAX_SPEED) particles[i].dx =  MAX_SPEED;
  if(particles[i].dx < -MAX_SPEED) particles[i].dx = -MAX_SPEED;
  if(particles[i].dy >  MAX_SPEED) particles[i].dy =  MAX_SPEED;
  if(particles[i].dy < -MAX_SPEED) particles[i].dy = -MAX_SPEED;

  particles[i].position.x += particles[i].dx;
  particles[i].position.y += particles[i].dy;

  if(particles[i].dx < 0.1F && particles[i].dy < 0.1F) {
    // particles[i].grav_center = random_point_roughly_in_screen();
  }
}

void draw_particle(GContext* ctx, int i) {
  // int size = max(1 + rand_between(-1,1), 0);
  graphics_fill_circle(ctx, particles[i].position, 1);
}

void update_square_layer(Layer *me, GContext* ctx) {
  (void)me;

  static unsigned int angle = 0;

  gpath_rotate_to(&square_path, (TRIG_MAX_ANGLE / 360) * angle);

  angle = (angle + 5) % 360;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  gpath_draw_outline(ctx, &square_path);

  // update debug text layer
  static char test_text[100];
  // int now = sin_lookup(angle * 32768 / 90);
  // int then = linearmap(now, -65536, 65536, 0, 10);


  // unsigned int foo = tinymt32_generate_uint32(&rndstate);
  xsprintf( test_text, "rand: %u", random_in_range(0,10));

  text_layer_set_text(&text_header_layer, test_text);

  graphics_context_set_fill_color(ctx, GColorWhite);
  // graphics_fill_circle(ctx, GPoint(window.layer.frame.size.w/2, window.layer.frame.size.h/2), then);
  for(int i=0;i<NUM_PARTICLES;i++) {
    update_particle(i);
    draw_particle(ctx, i);
  }
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

  if (cookie == COOKIE_MY_TIMER) {
    layer_mark_dirty(&square_layer);
    timer_handle = app_timer_send_event(ctx, 50 /* milliseconds */, COOKIE_MY_TIMER);
  }
}

void layer_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  (void)ctx;
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)t;
  (void)ctx;

  PblTm current_time;
  get_time(&current_time);
}

void init_particles() {
  for(int i=0; i<NUM_PARTICLES; i++) {
    GPoint start = random_point_roughly_in_screen();
    GPoint goal = GPoint(window.layer.frame.size.w/2, window.layer.frame.size.h/2);
    float initial_power = 100.0F;
    particles[i] = FParticle(start.x, start.y, 
                             goal.x, goal.y, 
                             initial_power);
  }
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  uint32_t seed = 0;
  tinymt32_init(&rndstate, seed);

  window_init(&window, "Fireflies");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  // resource_init_current_app(&APP_RESOURCES);

  // Init the layer for the minute display
  // layer_init(&layer, window.layer.frame);
  // layer.update_proc = &layer_update_callback;
  // layer_add_child(&window.layer, &layer);

  init_particles();

  // setup debugging text layer
  text_layer_init(&text_header_layer, window.layer.frame);
  text_layer_set_text_color(&text_header_layer, GColorWhite);
  text_layer_set_background_color(&text_header_layer, GColorClear);
  layer_set_frame(&text_header_layer.layer, GRect(0, 0, 144-0, 168-0));
  text_layer_set_font(&text_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(&window.layer, &text_header_layer.layer);
 
  layer_init(&square_layer, GRect(0,0, window.layer.frame.size.w, window.layer.frame.size.h));
  square_layer.update_proc = update_square_layer;
  layer_add_child(&window.layer, &square_layer);

  gpath_init(&square_path, &SQUARE_POINTS);
  gpath_move_to(&square_path, grect_center_point(&square_layer.frame));

  timer_handle = app_timer_send_event(ctx, 50 /* milliseconds */, COOKIE_MY_TIMER);
}

void handle_deinit(AppContextRef ctx) {
	(void)ctx;
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .timer_handler = &handle_timer,

    // Handle time updates
    .tick_info = {
      .tick_handler = &handle_tick,
      .tick_units = MINUTE_UNIT
    }

  };
  app_event_loop(params, &handlers);
}


