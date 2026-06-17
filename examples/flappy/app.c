#include "app.h"

#include "assets.h"
#include "renderer.h"

#include <lcom/i8042.h>

#include <stdio.h>

enum {
  STATE_MENU = 0,
  STATE_PLAYING = 1,
  STATE_GAME_OVER = 2
};

static int overlaps(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void reset_round(app_t *app) {
  app->bird_y = 250;
  app->bird_vy = 0;
  app->pipe_x = 820;
  app->gap_y = 210;
  app->score = 0;
  app->frame = 0;
}

static void finish_round(app_t *app) {
  if (app->score > app->best_score) app->best_score = app->score;
  app->state = STATE_GAME_OVER;
  menu_set(&app->menu, "GAME OVER", "RETRY", "QUIT");
}

static app_input_result_t start_round(app_t *app) {
  reset_round(app);
  app->state = STATE_PLAYING;
  app->bird_vy = -9;
  return APP_INPUT_FLAP;
}

static app_input_result_t handle_menu_action(app_t *app, menu_action_t action) {
  if (action == MENU_ACTION_PRIMARY) return start_round(app);
  if (action == MENU_ACTION_QUIT) {
    app->running = 0;
    return APP_INPUT_QUIT;
  }
  return APP_INPUT_NONE;
}

static const char *const *bird_sprite_for_vy(int vy) {
  if (vy < -7) return bird_climb_hard_xpm;
  if (vy < -1) return bird_climb_xpm;
  if (vy > 7) return bird_dive_hard_xpm;
  if (vy > 2) return bird_dive_xpm;
  return bird_flat_xpm;
}

void app_init(app_t *app) {
  reset_round(app);
  app->state = STATE_MENU;
  app->running = 1;
  app->best_score = 0;
  menu_init(&app->menu);
  menu_set(&app->menu, "LCOM BIRD", "PLAY", "QUIT");
}

app_input_result_t app_key(app_t *app, uint8_t scancode) {
  if (app->state == STATE_MENU || app->state == STATE_GAME_OVER) {
    return handle_menu_action(app, menu_key(&app->menu, scancode));
  }

  int make = (scancode & KBD_BREAK_BIT) == 0;
  uint8_t code = (uint8_t)(scancode & ~KBD_BREAK_BIT);
  if (!make) return scancode == ESC_BREAK ? APP_INPUT_QUIT : APP_INPUT_NONE;
  if (code == 0x39 || code == 0x48) {
    app->bird_vy = -9;
    return APP_INPUT_FLAP;
  }
  return APP_INPUT_NONE;
}

app_input_result_t app_mouse(app_t *app, int x, int y, int left_down) {
  if (app->state == STATE_MENU || app->state == STATE_GAME_OVER) {
    return handle_menu_action(app, menu_mouse(&app->menu, x, y, left_down));
  }

  app->menu.pointer_x = x;
  app->menu.pointer_y = y;
  int pressed = left_down && !app->menu.pointer_down;
  app->menu.pointer_down = left_down;
  if (pressed) {
    app->bird_vy = -9;
    return APP_INPUT_FLAP;
  }
  return APP_INPUT_NONE;
}

void app_update(app_t *app) {
  app->frame++;
  if (app->state != STATE_PLAYING) return;

  app->bird_vy += 1;
  if (app->bird_vy > 12) app->bird_vy = 12;
  app->bird_y += app->bird_vy;
  app->pipe_x -= 4;

  if (app->pipe_x < -80) {
    app->pipe_x = 820;
    app->gap_y = 140 + ((app->score * 47) % 260);
    app->score++;
  }

  int bird_x = 150;
  int bird_w = 48;
  int bird_h = 36;
  int gap_h = 150;
  if (app->bird_y < 0 || app->bird_y + bird_h > 560) finish_round(app);
  if (overlaps(bird_x, app->bird_y, bird_w, bird_h, app->pipe_x, 0, 72, app->gap_y) ||
      overlaps(bird_x, app->bird_y, bird_w, bird_h, app->pipe_x, app->gap_y + gap_h, 72, 600)) {
    finish_round(app);
  }
}

void app_render(app_t *app, draw_context_t *ctx) {
  clear_screen(ctx, 0x24201Au);
  fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height, 0x241E18u);
  fill_rect(ctx, 0, 520, ctx->info.width, 80, 0x3D8A4Cu);
  fill_rect(ctx, 0, 548, ctx->info.width, 52, 0x6FB45Du);

  int gap_h = 150;
  int bird_x = 150;
  draw_xpm(ctx, pipe_xpm, app->pipe_x, app->gap_y - 64 * 3, 3);
  draw_xpm(ctx, pipe_xpm, app->pipe_x, app->gap_y + gap_h, 3);
  draw_xpm(ctx, bird_sprite_for_vy(app->bird_vy), bird_x, app->bird_y, 3);

  char score[32];
  snprintf(score, sizeof(score), "SCORE %d", app->score);
  draw_text(ctx, 24, 24, score, 0xFFFFFFu, 3);

  if (app->state == STATE_MENU) {
    menu_set(&app->menu, "LCOM BIRD", "PLAY", "QUIT");
    menu_render(&app->menu, ctx, app->best_score);
  } else if (app->state == STATE_GAME_OVER) {
    menu_set(&app->menu, "GAME OVER", "RETRY", "QUIT");
    menu_render(&app->menu, ctx, app->best_score);
  }
}
