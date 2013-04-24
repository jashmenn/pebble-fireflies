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
#define NUM_PARTICLES 140
#define COOKIE_MY_TIMER 1
#define COOKIE_SWARM_TIMER 2
#define maximum(a,b) a > b ? a : b
#define minimum(a,b) ((a) < (b) ? (a) : (b))
#define NORMAL_POWER 300.0F
#define TIGHT_POWER 1.0F

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
Layer particle_layer;
TextLayer text_header_layer;
AppTimerHandle timer_handle;
tinymt32_t rndstate;
int showing_time = 0;

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

static const GBitmap* number_bitmaps[10] = { 
  &s_0_bitmap, &s_1_bitmap, &s_2_bitmap, 
  &s_3_bitmap, &s_4_bitmap, &s_5_bitmap,
  &s_6_bitmap, &s_7_bitmap, &s_8_bitmap, 
  &s_9_bitmap 
};

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

  if(showing_time == 0) {
    if((abs(particles[i].size - MIN_SIZE) < 0.001F) && (tinymt32_generate_float01(&rndstate) < 0.0008F)) {
      particles[i].goal_size = MAX_SIZE;
    }

    if(abs(particles[i].size - MAX_SIZE) < 0.001F) {
      particles[i].goal_size = MIN_SIZE;
    }
  }

  particles[i].ds += -(particles[i].size - particles[i].goal_size)/random_in_rangef(1000.0F, 10000.0F);
  if(abs(particles[i].size - particles[i].goal_size) > 0.01) {
    particles[i].size += particles[i].ds;
  }
  if(particles[i].size > MAX_SIZE) particles[i].size = MAX_SIZE;
  if(particles[i].size < MIN_SIZE) particles[i].size = MIN_SIZE;
}

void draw_particle(GContext* ctx, int i) {
  graphics_fill_circle(ctx, GPoint((int)particles[i].position.x,
                                   (int)particles[i].position.y),
                       particles[i].size);
}

void update_particles_layer(Layer *me, GContext* ctx) {
  (void)me;
  static unsigned int angle = 0;

  // update debug text layer
  static char test_text[100];
  // unsigned int foo = tinymt32_generate_uint32(&rndstate);
  // xsprintf( test_text, "rand: %u", random_in_range(0,10));
  // text_layer_set_text(&text_header_layer, test_text);

  graphics_context_set_fill_color(ctx, GColorWhite);
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
     layer_mark_dirty(&particle_layer);
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

void swarm_to_digit(int digit, int start_idx, int end_idx, int offset_x, int offset_y) {
  int end = minimum(end_idx, NUM_PARTICLES);

  for(int i=start_idx; i<end; i++) {

    GBitmap bitmap   = *number_bitmaps[digit];
    const uint8_t *pixels = bitmap.addr;

    // pick a random point in the four image
    // get the image
    // pick a random number the length of the array
    // if the value is zero, pick a differnet number increment until you get there
    // move to that position
    int image_length = bitmap.row_size_bytes * bitmap.bounds.size.h - 1;
    int idx = random_in_range(0, image_length);
    uint8_t pixel = pixels[idx];

    // guarantee we're on a non-zero byte
    while(pixel == 0x00) {
     idx = random_in_range(0, image_length);
     pixel = pixels[idx];
    }

    // pick a non-zero bit
    int bit_pos = (idx * 8);
    int bit_add = random_in_range(0,7);
    int tries = 8;
    while(!(pixel & (1 << bit_add))) {
      if(tries < 0) break;
      bit_add = random_in_range(0,7);
      tries--;
    }
    bit_pos += bit_add;

    
    int row_size_bits = bitmap.row_size_bytes * 8;
    int pixel_row = bit_pos / row_size_bits;
    int pixel_col = bit_pos % row_size_bits;

    float scale = 1.0F;
    GPoint goal = GPoint(scale*pixel_col+offset_x, scale*pixel_row+offset_y); // switch row & col

    particles[i].grav_center = FPoint(goal.x, goal.y);
    particles[i].power = TIGHT_POWER;
    particles[i].goal_size = random_in_rangef(2.0F, 3.5F);
  }

}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) { return hour; }
  unsigned short display_hour = hour % 12;
  return display_hour ? display_hour : 12; // Converts "0" to "12"
}

void display_time(PblTm *tick_time) {
  showing_time = 1;
  unsigned short hour = get_display_hour(tick_time->tm_hour);
  int min = tick_time->tm_min;

  //int particles_per_group = NUM_PARTICLES / 2;

  int hr_digit_tens = hour / 10; 
  int hr_digit_ones = hour % 10; 
  int min_digit_tens = min / 10; 
  int min_digit_ones = min % 10; 
  int w = window.layer.frame.size.w;
  int h = window.layer.frame.size.h;

  // take out 5 particles
  // 2 for colon
  // 3 for floaters
  int save = 5;

  if(hr_digit_tens == 0) {
    int particles_per_group = (NUM_PARTICLES - save)/ 3;
    swarm_to_digit(hr_digit_ones,                          0, particles_per_group,   25, 60);
    swarm_to_digit(min_digit_tens,     particles_per_group, particles_per_group*2,   65, 60);
    swarm_to_digit(min_digit_ones, (particles_per_group*2), NUM_PARTICLES - save,    95, 60);

    // top colon
    particles[NUM_PARTICLES-2].grav_center = FPoint(57, 69);
    particles[NUM_PARTICLES-2].power = TIGHT_POWER;
    particles[NUM_PARTICLES-2].goal_size = 3.0F;

    // bottom colon
    particles[NUM_PARTICLES-1].grav_center = FPoint(57, 89);
    particles[NUM_PARTICLES-1].power = TIGHT_POWER;
    particles[NUM_PARTICLES-1].goal_size = 3.0F;

  } else {
    int particles_per_group = (NUM_PARTICLES - save)/ 4;
    swarm_to_digit(hr_digit_tens,                          0, particles_per_group,   10, 60);
    swarm_to_digit(hr_digit_ones,      particles_per_group, particles_per_group*2, 40, 60);
    swarm_to_digit(min_digit_tens, (particles_per_group*2), particles_per_group*3, 80, 60);
    swarm_to_digit(min_digit_ones, (particles_per_group*3), NUM_PARTICLES - save,  110, 60);

    // top colon
    particles[NUM_PARTICLES-2].grav_center = FPoint(68, 69);
    particles[NUM_PARTICLES-2].power = TIGHT_POWER;
    particles[NUM_PARTICLES-2].goal_size = 3.0F;

    // bottom colon
    particles[NUM_PARTICLES-1].grav_center = FPoint(68, 89);
    particles[NUM_PARTICLES-1].power = TIGHT_POWER;
    particles[NUM_PARTICLES-1].goal_size = 3.0F;
  }

}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)t;
  (void)ctx;

  PblTm current_time;
  get_time(&current_time);
  display_time(&current_time);
}


void init_particles() {
  for(int i=0; i<NUM_PARTICLES; i++) {

    GPoint start = random_point_roughly_in_screen(10, 0);
    GPoint goal = GPoint(window.layer.frame.size.w/2, window.layer.frame.size.h/2);

    // GPoint start = goal;
    float initial_power = 300.0F;
    // float initial_power = 1.0F;
    particles[i] = FParticle(start.x, start.y, 
                             goal.x, goal.y, 
                             initial_power);
    particles[i].size = particles[i].goal_size = 0.0F;
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
 
  layer_init(&particle_layer, GRect(0,0, window.layer.frame.size.w, window.layer.frame.size.h));
  particle_layer.update_proc = update_particles_layer;
  layer_add_child(&window.layer, &particle_layer);

  gpath_init(&square_path, &SQUARE_POINTS);
  gpath_move_to(&square_path, grect_center_point(&particle_layer.frame));

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


