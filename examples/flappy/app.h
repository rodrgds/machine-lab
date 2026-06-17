#ifndef FLAPPY_APP_H
#define FLAPPY_APP_H

#include "font.h"
#include "menu.h"

typedef enum {
  APP_INPUT_NONE = 0,
  APP_INPUT_FLAP,
  APP_INPUT_QUIT
} app_input_result_t;

typedef struct {
  int state;
  int bird_y;
  int bird_vy;
  int pipe_x;
  int gap_y;
  int score;
  int frame;
  int running;
  int best_score;
  menu_t menu;
} app_t;

void app_init(app_t *app);
app_input_result_t app_key(app_t *app, uint8_t scancode);
app_input_result_t app_mouse(app_t *app, int x, int y, int left_down);
void app_update(app_t *app);
void app_render(app_t *app, draw_context_t *ctx);

#endif
