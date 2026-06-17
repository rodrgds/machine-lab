#include "menu.h"

#include "renderer.h"

#include <lcom/i8042.h>

#include <stdio.h>

enum {
  KEY_ENTER = 0x1C,
  KEY_UP = 0x48,
  KEY_DOWN = 0x50,
  KEY_SPACE = 0x39,
  BUTTON_X = 270,
  BUTTON_W = 260,
  BUTTON_H = 52,
  BUTTON_PRIMARY_Y = 286,
  BUTTON_SECONDARY_Y = 352
};

static int point_in_rect(int px, int py, int x, int y, int w, int h) {
  return px >= x && py >= y && px < x + w && py < y + h;
}

static int hovered_button(menu_t *menu) {
  if (point_in_rect(menu->pointer_x, menu->pointer_y, BUTTON_X, BUTTON_PRIMARY_Y,
                    BUTTON_W, BUTTON_H)) {
    return 0;
  }
  if (point_in_rect(menu->pointer_x, menu->pointer_y, BUTTON_X, BUTTON_SECONDARY_Y,
                    BUTTON_W, BUTTON_H)) {
    return 1;
  }
  return -1;
}

static void draw_button(draw_context_t *ctx, int y, const char *label, int selected) {
  uint32_t bg = selected ? 0xF2C94Cu : 0x313A46u;
  uint32_t shadow = selected ? 0xB98323u : 0x18202Au;
  uint32_t text = selected ? 0x1E2329u : 0xEFF6FFu;
  fill_rect(ctx, BUTTON_X + 4, y + 5, BUTTON_W, BUTTON_H, shadow);
  fill_rect(ctx, BUTTON_X, y, BUTTON_W, BUTTON_H, bg);
  fill_rect(ctx, BUTTON_X, y, BUTTON_W, 4, selected ? 0xFFE082u : 0x4D5B6Au);
  fill_rect(ctx, BUTTON_X, y + BUTTON_H - 4, BUTTON_W, 4, selected ? 0xB98323u : 0x222C36u);

  int text_x = BUTTON_X + (BUTTON_W - (int)font_text_width(label, 3)) / 2;
  draw_text(ctx, text_x, y + 15, label, text, 3);
}

void menu_init(menu_t *menu) {
  menu->title = "LCOM BIRD";
  menu->primary_label = "PLAY";
  menu->secondary_label = "QUIT";
  menu->selected = 0;
  menu->pointer_x = 400;
  menu->pointer_y = 312;
  menu->pointer_down = 0;
}

void menu_set(menu_t *menu, const char *title, const char *primary, const char *secondary) {
  menu->title = title;
  menu->primary_label = primary;
  menu->secondary_label = secondary;
}

menu_action_t menu_key(menu_t *menu, uint8_t scancode) {
  int make = (scancode & KBD_BREAK_BIT) == 0;
  uint8_t code = (uint8_t)(scancode & ~KBD_BREAK_BIT);
  if (!make) return scancode == ESC_BREAK ? MENU_ACTION_QUIT : MENU_ACTION_NONE;

  if (code == KEY_UP || code == KEY_DOWN) {
    menu->selected = 1 - menu->selected;
    return MENU_ACTION_NONE;
  }
  if (code == KEY_SPACE || code == KEY_ENTER) {
    return menu->selected == 0 ? MENU_ACTION_PRIMARY : MENU_ACTION_QUIT;
  }
  return MENU_ACTION_NONE;
}

menu_action_t menu_mouse(menu_t *menu, int x, int y, int left_down) {
  menu->pointer_x = x;
  menu->pointer_y = y;
  int hovered = hovered_button(menu);
  if (hovered >= 0) menu->selected = hovered;

  menu_action_t action = MENU_ACTION_NONE;
  if (left_down && !menu->pointer_down && hovered >= 0) {
    action = hovered == 0 ? MENU_ACTION_PRIMARY : MENU_ACTION_QUIT;
  }
  menu->pointer_down = left_down;
  return action;
}

void menu_render(menu_t *menu, draw_context_t *ctx, int score) {
  int title_x = (ctx->info.width - (int)font_text_width(menu->title, 6)) / 2;
  draw_text(ctx, title_x, 176, menu->title, 0x4CC9F0u, 6);

  if (score > 0) {
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "BEST RUN %d", score);
    int score_x = (ctx->info.width - (int)font_text_width(score_text, 2)) / 2;
    draw_text(ctx, score_x, 246, score_text, 0xB8C2CCu, 2);
  }

  draw_button(ctx, BUTTON_PRIMARY_Y, menu->primary_label, menu->selected == 0);
  draw_button(ctx, BUTTON_SECONDARY_Y, menu->secondary_label, menu->selected == 1);

  fill_rect(ctx, menu->pointer_x, menu->pointer_y, 13, 4, 0xFFFFFFu);
  fill_rect(ctx, menu->pointer_x, menu->pointer_y, 4, 13, 0xFFFFFFu);
  fill_rect(ctx, menu->pointer_x + 2, menu->pointer_y + 2, 7, 2, 0x101820u);
}
