#include <stdio.h>
#include <math.h>
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "xprintf.h"
#include "common.h"
#include "tinymt32.h"
#include "numbers.h"

// defines
#define MY_UUID { 0x74, 0x19, 0xF2, 0x5C, 0x82, 0x1B, 0x4B, 0xD8, 0x93, 0xD5, 0x98, 0x7E, 0x31, 0x15, 0xA8, 0xB6 }
PBL_APP_INFO(MY_UUID,
             "Fireflies", "Little Hiccup",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);
#define NUM_PARTICLES 200
#define COOKIE_MY_TIMER 1
#define COOKIE_SWARM_TIMER 2
#define max(a,b) a > b ? a : b
#define minimum(a,b) ((a) < (b) ? (a) : (b))

typedef struct FPoint
{
  float x;
  float y;
} FPoint;

// typedefs
typedef struct FParticle 
{
  FPoint position;
  FPoint grav_center;
  float dx;
  float dy;
  float power;
  float size;
  float goal_size;
  float ds;
} FParticle;
#define FParticle(px, py, gx, gy, power) ((FParticle){{(px), (py)}, {(gx), (gy)}, 0.0F, 0.0F, power, 0.0F, 0.0F, 0.0F})
#define FPoint(x, y) ((FPoint){(x), (y)})

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

#define MAX_SPEED      1.0F

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
GPoint random_point_roughly_in_screen(int margin, int padding) {
  return GPoint(random_in_range(0-margin+padding, window.layer.frame.size.w+1+margin-padding), 
                random_in_range(0-margin+padding, window.layer.frame.size.h+1+margin-padding));
}

#define JITTER 0.5F
#define MAX_SIZE 3.0F
#define MIN_SIZE 0.0F

void update_particle(int i) {
  // 
  if(tinymt32_generate_float01(&rndstate) < 0.4F) {
    particles[i].dx += random_in_rangef(-JITTER, JITTER);
    particles[i].dy += random_in_rangef(-JITTER, JITTER);
  }

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

  // update size
  if((abs(particles[i].size - MIN_SIZE) < 0.001F) && (tinymt32_generate_float01(&rndstate) < 0.0008F)) {
    particles[i].goal_size = MAX_SIZE;
  }

  if(abs(particles[i].size - MAX_SIZE) < 0.001F) {
    particles[i].goal_size = MIN_SIZE;
  }

  particles[i].ds += -(particles[i].size - particles[i].goal_size)/30.0F;
  particles[i].size += particles[i].ds;
  if(particles[i].size > MAX_SIZE) particles[i].size = MAX_SIZE;
  if(particles[i].size < MIN_SIZE) particles[i].size = MIN_SIZE;
}

void draw_particle(GContext* ctx, int i) {
  graphics_fill_circle(ctx, GPoint((int)particles[i].position.x,
                                   (int)particles[i].position.y),
                       particles[i].size);
}

void update_square_layer(Layer *me, GContext* ctx) {
  (void)me;

  static unsigned int angle = 0;

  gpath_rotate_to(&square_path, (TRIG_MAX_ANGLE / 360) * angle);

  angle = (angle + 5) % 360;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  // gpath_draw_outline(ctx, &square_path);

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

void swarm_to_a_different_location() {
  GPoint new_gravity = random_point_in_screen();
  FPoint new_gravityf = FPoint(new_gravity.x, new_gravity.y);
  for(int i=0;i<NUM_PARTICLES;i++) {
    particles[i].grav_center = new_gravityf;
  }
}


void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

  if (cookie == COOKIE_MY_TIMER) {
    layer_mark_dirty(&square_layer);
    timer_handle = app_timer_send_event(ctx, 50 /* milliseconds */, COOKIE_MY_TIMER);
  } else if (cookie == COOKIE_SWARM_TIMER) {
    // swarm_to_a_different_location();
    app_timer_send_event(ctx, random_in_range(5000,15000) /* milliseconds */, COOKIE_SWARM_TIMER);
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
    GPoint start = random_point_roughly_in_screen(10, 0);
    // GPoint goal = GPoint(window.layer.frame.size.w/2, window.layer.frame.size.h/2);

    // pick a random point in the four image
    // get the image
    // pick a random number the length of the array
    // if the value is zero, pick a differnet number increment until you get there
    // move to that position
    int image_length = s_4_bitmap.row_size_bytes * s_4_bitmap.bounds.size.h - 1;
    int idx = random_in_range(0, image_length);
    uint8_t pixel = s_4_pixels[idx];

    // guarantee we're on a non-zero pixel
    while(pixel == 0x00) {
     idx = random_in_range(0, image_length);
     pixel = s_4_pixels[idx];
    }

    // int bit_pos = (idx * 8) + __builtin_ctz(pixel); // e.g. 00011000 returns 3
    //int bit_pos = (idx * 8) + __builtin_ctz(pixel); // e.g. 00011000 returns 3
    //int bit_pos = (idx * 8) + __builtin_clz(pixel) - 24; // e.g. 00011000 returns 3
    //int bit_pos = (idx * 8) + random_in_range( __builtin_ctz(pixel), __builtin_clz(pixel) - 24,);
    //int bit_pos = (idx * 8) + random_in_range(__builtin_clz(pixel) - 24, 7 - __builtin_ctz(pixel)) - 1;
    //int bit_pos = (idx * 8) + random_in_range(0, 7);
    int bit_pos = (idx * 8);// + random_in_range(0, 7);
    int bit_add = random_in_range(0,7);
    int tries = 8;
    while(!(pixel & (1 << bit_add))) {
      if(tries < 0) break;
      bit_add = random_in_range(0,7);
      tries--;
    }
    bit_pos += bit_add;
    
    int row_size_bits = s_4_bitmap.row_size_bytes * 8;
    int pixel_row = bit_pos / row_size_bits;
    int pixel_col = bit_pos % row_size_bits;

    int origin = 30;
    //GPoint goal = GPoint(pixel_row+origin, pixel_col+origin);
    GPoint goal = GPoint(pixel_col+origin, pixel_row+origin);
    //GPoint start = goal;

    // 0000 
    // 0001
    // 0000
    // 
    // idx: 7
    // (1,1)
    // bc
    // 
    // row size = 4
    // row = idx / row_size
    // 1 = 7 / 4
    // col = idx % row_size
    // 3 = 7 % 4

    // 000
    // 000
    // 010
    // idx: 7
    // row = idx / row_size
    // 2  = 7 / 3
    // col = idx % row_size
    // 1  =  7 % 3

    //  012345678
    //  000000000 0
    //  000000000 1
    //  000000100 2

    // (6,2)
    // idx = 24
    //   2 = 24 / 9
    //   6 

    // float initial_power = 300.0F;
    float initial_power = 1.0F;
    particles[i] = FParticle(start.x, start.y, 
                             goal.x, goal.y, 
                             initial_power);
    particles[i].size = particles[i].goal_size = 1.0F;
  }
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  uint32_t seed = 4;
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
  app_timer_send_event(ctx, random_in_range(5000,15000) /* milliseconds */, COOKIE_SWARM_TIMER);
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


