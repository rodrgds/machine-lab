#ifndef FLAPPY_MENU_H
#define FLAPPY_MENU_H

#include "font.h"

#include <stdint.h>

typedef enum {
  MENU_ACTION_NONE = 0,
  MENU_ACTION_PRIMARY,
  MENU_ACTION_QUIT
} menu_action_t;

typedef struct {
  const char *title;
  const char *primary_label;
  const char *secondary_label;
  int selected;
  int pointer_x;
  int pointer_y;
  int pointer_down;
} menu_t;

void menu_init(menu_t *menu);
void menu_set(menu_t *menu, const char *title, const char *primary, const char *secondary);
menu_action_t menu_key(menu_t *menu, uint8_t scancode);
menu_action_t menu_mouse(menu_t *menu, int x, int y, int left_down);
void menu_render(menu_t *menu, draw_context_t *ctx, int score);

#endif
