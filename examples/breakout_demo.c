#include "example_ui.h"

#include <lcom/i8042.h>
#include <lcom/i8254.h>
#include <lcom/lcom.h>
#include <lcom/vbe.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define BRICK_ROWS 4
#define BRICK_COLS 9

typedef struct {
  int paddle_x;
  int ball_x;
  int ball_y;
  int ball_vx;
  int ball_vy;
  uint8_t bricks[BRICK_ROWS][BRICK_COLS];
  uint32_t score;
  bool left_down;
  bool right_down;
} breakout_state_t;

static void init_breakout(breakout_state_t *state) {
  state->paddle_x = 340;
  state->ball_x = 388;
  state->ball_y = 404;
  state->ball_vx = 4;
  state->ball_vy = -4;
  state->score = 0;
  state->left_down = false;
  state->right_down = false;
  for (uint8_t row = 0; row < BRICK_ROWS; row++) {
    for (uint8_t col = 0; col < BRICK_COLS; col++) {
      state->bricks[row][col] = 1;
    }
  }
}

static void reset_ball(breakout_state_t *state) {
  state->ball_x = state->paddle_x + 48;
  state->ball_y = 404;
  state->ball_vx = state->ball_vx < 0 ? -4 : 4;
  state->ball_vy = -4;
}

static void update_breakout(breakout_state_t *state) {
  if (state->left_down) state->paddle_x -= 7;
  if (state->right_down) state->paddle_x += 7;
  if (state->paddle_x < 54) state->paddle_x = 54;
  if (state->paddle_x > 650) state->paddle_x = 650;

  state->ball_x += state->ball_vx;
  state->ball_y += state->ball_vy;

  if (state->ball_x < 48 || state->ball_x > 744) state->ball_vx = -state->ball_vx;
  if (state->ball_y < 94) state->ball_vy = -state->ball_vy;

  if (state->ball_y >= 448 && state->ball_y <= 466 &&
      state->ball_x >= state->paddle_x && state->ball_x <= state->paddle_x + 96 &&
      state->ball_vy > 0) {
    state->ball_vy = -state->ball_vy;
    int hit = state->ball_x - state->paddle_x;
    state->ball_vx = hit < 32 ? -5 : (hit > 64 ? 5 : state->ball_vx);
  }

  const int brick_x = 68;
  const int brick_y = 124;
  const int brick_w = 72;
  const int brick_h = 28;
  for (uint8_t row = 0; row < BRICK_ROWS; row++) {
    for (uint8_t col = 0; col < BRICK_COLS; col++) {
      if (state->bricks[row][col] == 0) continue;
      int x = brick_x + col * brick_w;
      int y = brick_y + row * brick_h;
      if (state->ball_x >= x && state->ball_x <= x + brick_w - 8 &&
          state->ball_y >= y && state->ball_y <= y + brick_h - 6) {
        state->bricks[row][col] = 0;
        state->ball_vy = -state->ball_vy;
        state->score += 10;
        return;
      }
    }
  }

  if (state->ball_y > 540) reset_ball(state);
}

static void draw_breakout(example_canvas_t *ctx, const breakout_state_t *state, uint32_t frame) {
  example_fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height,
                    example_rgb(22, 29, 35));
  example_fill_rect(ctx, 0, 0, ctx->info.width, 76, example_rgb(17, 101, 97));
  example_draw_text(ctx, 44, 26, "BREAKOUT LAB", example_rgb(239, 246, 238), 4);

  char score_text[32];
  snprintf(score_text, sizeof(score_text), "SCORE %u", state->score);
  example_draw_text(ctx, 574, 32, score_text, example_rgb(224, 241, 232), 2);

  example_stroke_rect(ctx, 44, 92, 712, 444, 4, example_rgb(78, 94, 105));
  const int brick_x = 68;
  const int brick_y = 124;
  const int brick_w = 72;
  const int brick_h = 28;
  for (uint8_t row = 0; row < BRICK_ROWS; row++) {
    for (uint8_t col = 0; col < BRICK_COLS; col++) {
      if (state->bricks[row][col] == 0) continue;
      uint32_t color = row == 0 ? example_rgb(238, 119, 91)
                       : row == 1 ? example_rgb(238, 174, 85)
                       : row == 2 ? example_rgb(89, 171, 136)
                                  : example_rgb(91, 143, 210);
      example_fill_rect(ctx, brick_x + col * brick_w, brick_y + row * brick_h,
                        brick_w - 8, brick_h - 6, color);
    }
  }

  example_fill_rect(ctx, state->paddle_x, 454, 96, 15, example_rgb(232, 238, 243));
  example_fill_rect(ctx, state->paddle_x + 8, 448, 80, 8, example_rgb(117, 211, 196));
  example_fill_rect(ctx, state->ball_x - 7, state->ball_y - 7, 14, 14,
                    example_rgb(248, 239, 117));
  if ((frame % 40u) < 20u) {
    example_draw_text(ctx, 184, 560, "TIMER IRQ GAME LOOP  KEYBOARD IRQ PADDLE",
                      example_rgb(178, 193, 202), 2);
  }
}

static void handle_extended_key(uint8_t code, bool break_code, breakout_state_t *state) {
  bool pressed = !break_code;
  if (code == 0x4B) state->left_down = pressed;
  if (code == 0x4D) state->right_down = pressed;
}

static void handle_key(uint8_t data, bool *extended, breakout_state_t *state, bool *running) {
  if (data == KBD_TWO_BYTE_PFX) {
    *extended = true;
    return;
  }

  bool break_code = (data & KBD_BREAK_BIT) != 0;
  uint8_t code = (uint8_t)(data & 0x7Fu);
  if (*extended) {
    handle_extended_key(code, break_code, state);
    *extended = false;
    return;
  }

  if (data == ESC_BREAK) *running = false;
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;

  lcom_vbe_mode_info_t info;
  if (lcom_vbe_get_mode_info(LCOM_VBE_MODE_800_600_24, &info) != LCOM_OK) return 1;
  if (lcom_vbe_set_mode(LCOM_VBE_MODE_800_600_24) != LCOM_OK) return 1;

  uint8_t *fb = 0;
  if (lcom_phys_map(info.framebuffer_phys, info.framebuffer_size, (void **)&fb) != LCOM_OK) {
    return 1;
  }

  lcom_irq_t timer_irq;
  lcom_irq_t kbd_irq;
  if (lcom_irq_subscribe(TIMER0_IRQ, 0, &timer_irq) != LCOM_OK) return 1;
  if (lcom_irq_subscribe(KBC_IRQ, 0, &kbd_irq) != LCOM_OK) return 1;

  breakout_state_t state;
  init_breakout(&state);
  example_canvas_t canvas = {fb, info};
  bool extended = false;
  bool running = true;
  uint32_t frame = 0;

  draw_breakout(&canvas, &state, frame);
  lcom_vbe_present();

  while (running) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) break;

    if ((ev.irq_mask & kbd_irq.mask) != 0) {
      uint8_t status = 0;
      uint8_t data = 0;
      if (lcom_port_read8(KBC_ST_REG, &status) == LCOM_OK &&
          (status & KBC_ST_OBF) != 0 &&
          (status & KBC_ST_AUX) == 0 &&
          lcom_port_read8(KBC_OUT_BUF, &data) == LCOM_OK) {
        handle_key(data, &extended, &state, &running);
      }
    }

    if ((ev.irq_mask & timer_irq.mask) != 0) {
      frame++;
      update_breakout(&state);
      draw_breakout(&canvas, &state, frame);
      lcom_vbe_present();
    }
  }

  draw_breakout(&canvas, &state, frame);
  lcom_vbe_present();
  lcom_printf("breakout demo score=%u frames=%u\n", state.score, frame);

  lcom_irq_unsubscribe(&kbd_irq);
  lcom_irq_unsubscribe(&timer_irq);
  lcom_phys_unmap(fb, info.framebuffer_size);
  lcom_exit();
  return 0;
}
