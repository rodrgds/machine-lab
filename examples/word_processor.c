#include "example_ui.h"

#include <lcom/i8042.h>
#include <lcom/i8254.h>
#include <lcom/lcom.h>
#include <lcom/vbe.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define EDITOR_MAX_TEXT 4096u
#define EDITOR_WRAP_COLS 46u
#define EDITOR_VISIBLE_ROWS 19u

typedef struct {
  char text[EDITOR_MAX_TEXT + 1u];
  size_t len;
  size_t cursor;
  uint16_t scroll_line;
  bool extended;
  bool overwrite;
  bool dirty;
} editor_state_t;

typedef struct {
  uint16_t line;
  uint16_t col;
} visual_pos_t;

static char scancode_to_char(uint8_t code) {
  switch (code) {
  case 0x02: return '1';
  case 0x03: return '2';
  case 0x04: return '3';
  case 0x05: return '4';
  case 0x06: return '5';
  case 0x07: return '6';
  case 0x08: return '7';
  case 0x09: return '8';
  case 0x0A: return '9';
  case 0x0B: return '0';
  case 0x0C: return '-';
  case 0x0D: return '=';
  case 0x10: return 'Q';
  case 0x11: return 'W';
  case 0x12: return 'E';
  case 0x13: return 'R';
  case 0x14: return 'T';
  case 0x15: return 'Y';
  case 0x16: return 'U';
  case 0x17: return 'I';
  case 0x18: return 'O';
  case 0x19: return 'P';
  case 0x1A: return '[';
  case 0x1B: return ']';
  case 0x1E: return 'A';
  case 0x1F: return 'S';
  case 0x20: return 'D';
  case 0x21: return 'F';
  case 0x22: return 'G';
  case 0x23: return 'H';
  case 0x24: return 'J';
  case 0x25: return 'K';
  case 0x26: return 'L';
  case 0x27: return ';';
  case 0x28: return '\'';
  case 0x2B: return '\\';
  case 0x2C: return 'Z';
  case 0x2D: return 'X';
  case 0x2E: return 'C';
  case 0x2F: return 'V';
  case 0x30: return 'B';
  case 0x31: return 'N';
  case 0x32: return 'M';
  case 0x33: return ',';
  case 0x34: return '.';
  case 0x35: return '/';
  case 0x39: return ' ';
  default: return 0;
  }
}

static int read_keyboard_byte(uint8_t *data) {
  uint8_t status = 0;
  if (lcom_port_read8(KBC_ST_REG, &status) != LCOM_OK) return LCOM_ERR;
  if ((status & KBC_ST_OBF) == 0 || (status & KBC_ST_AUX) != 0) return LCOM_NO_DATA;
  return lcom_port_read8(KBC_OUT_BUF, data);
}

static void visual_position_at(const editor_state_t *ed, size_t index, visual_pos_t *out) {
  uint16_t line = 0;
  uint16_t col = 0;
  if (index > ed->len) index = ed->len;

  for (size_t i = 0; i < index; i++) {
    if (ed->text[i] == '\n') {
      line++;
      col = 0;
      continue;
    }
    col++;
    if (col >= EDITOR_WRAP_COLS) {
      line++;
      col = 0;
    }
  }

  out->line = line;
  out->col = col;
}

static size_t index_for_visual_position(const editor_state_t *ed, uint16_t target_line,
                                        uint16_t target_col) {
  uint16_t line = 0;
  uint16_t col = 0;

  for (size_t i = 0; i < ed->len; i++) {
    if (line == target_line && col >= target_col) return i;
    if (ed->text[i] == '\n') {
      if (line == target_line) return i;
      line++;
      col = 0;
      continue;
    }
    col++;
    if (col >= EDITOR_WRAP_COLS) {
      line++;
      col = 0;
    }
  }

  return ed->len;
}

static uint16_t total_visual_lines(const editor_state_t *ed) {
  visual_pos_t end;
  visual_position_at(ed, ed->len, &end);
  return (uint16_t)(end.line + 1u);
}

static uint16_t word_count(const editor_state_t *ed) {
  uint16_t words = 0;
  bool in_word = false;
  for (size_t i = 0; i < ed->len; i++) {
    bool space = ed->text[i] == ' ' || ed->text[i] == '\n' || ed->text[i] == '\t';
    if (space) {
      in_word = false;
    } else if (!in_word) {
      words++;
      in_word = true;
    }
  }
  return words;
}

static void ensure_cursor_visible(editor_state_t *ed) {
  visual_pos_t pos;
  visual_position_at(ed, ed->cursor, &pos);
  if (pos.line < ed->scroll_line) {
    ed->scroll_line = pos.line;
  } else if (pos.line >= ed->scroll_line + EDITOR_VISIBLE_ROWS) {
    ed->scroll_line = (uint16_t)(pos.line - EDITOR_VISIBLE_ROWS + 1u);
  }
}

static void insert_char(editor_state_t *ed, char c) {
  if (ed->len >= EDITOR_MAX_TEXT) return;
  if (ed->overwrite && ed->cursor < ed->len && ed->text[ed->cursor] != '\n') {
    ed->text[ed->cursor++] = c;
  } else {
    memmove(&ed->text[ed->cursor + 1u], &ed->text[ed->cursor], ed->len - ed->cursor + 1u);
    ed->text[ed->cursor++] = c;
    ed->len++;
  }
  ed->dirty = true;
  ensure_cursor_visible(ed);
}

static void delete_at_cursor(editor_state_t *ed) {
  if (ed->cursor >= ed->len) return;
  memmove(&ed->text[ed->cursor], &ed->text[ed->cursor + 1u], ed->len - ed->cursor);
  ed->len--;
  ed->dirty = true;
  ensure_cursor_visible(ed);
}

static void backspace(editor_state_t *ed) {
  if (ed->cursor == 0) return;
  ed->cursor--;
  delete_at_cursor(ed);
}

static void move_cursor_line(editor_state_t *ed, int delta) {
  visual_pos_t pos;
  visual_position_at(ed, ed->cursor, &pos);
  int next_line = (int)pos.line + delta;
  if (next_line < 0) next_line = 0;
  ed->cursor = index_for_visual_position(ed, (uint16_t)next_line, pos.col);
  ensure_cursor_visible(ed);
}

static void move_cursor_home(editor_state_t *ed) {
  visual_pos_t pos;
  visual_position_at(ed, ed->cursor, &pos);
  ed->cursor = index_for_visual_position(ed, pos.line, 0);
  ensure_cursor_visible(ed);
}

static void move_cursor_end(editor_state_t *ed) {
  visual_pos_t pos;
  visual_position_at(ed, ed->cursor, &pos);
  size_t next = index_for_visual_position(ed, (uint16_t)(pos.line + 1u), 0);
  if (next > 0 && next <= ed->len && ed->text[next - 1u] == '\n') next--;
  ed->cursor = next;
  ensure_cursor_visible(ed);
}

static void handle_extended_key(editor_state_t *ed, uint8_t code, bool break_code) {
  if (break_code) return;
  switch (code) {
  case 0x47:
    move_cursor_home(ed);
    break;
  case 0x4F:
    move_cursor_end(ed);
    break;
  case 0x48:
    move_cursor_line(ed, -1);
    break;
  case 0x50:
    move_cursor_line(ed, 1);
    break;
  case 0x4B:
    if (ed->cursor > 0) ed->cursor--;
    ensure_cursor_visible(ed);
    break;
  case 0x4D:
    if (ed->cursor < ed->len) ed->cursor++;
    ensure_cursor_visible(ed);
    break;
  case 0x52:
    ed->overwrite = !ed->overwrite;
    break;
  case 0x53:
    delete_at_cursor(ed);
    break;
  default:
    break;
  }
}

static void handle_keyboard_byte(editor_state_t *ed, uint8_t data, bool *running) {
  if (data == KBD_TWO_BYTE_PFX) {
    ed->extended = true;
    return;
  }

  bool break_code = (data & KBD_BREAK_BIT) != 0;
  uint8_t code = (uint8_t)(data & 0x7Fu);

  if (ed->extended) {
    handle_extended_key(ed, code, break_code);
    ed->extended = false;
    return;
  }

  if (data == ESC_BREAK) {
    *running = false;
    return;
  }
  if (break_code) return;

  if (code == 0x0E) {
    backspace(ed);
  } else if (code == 0x1C) {
    insert_char(ed, '\n');
  } else if (code == 0x0F) {
    insert_char(ed, ' ');
    insert_char(ed, ' ');
  } else {
    char c = scancode_to_char(code);
    if (c != 0) insert_char(ed, c);
  }
}

static void draw_visible_text(example_canvas_t *ctx, const editor_state_t *ed, uint32_t frame,
                              int doc_x, int doc_y) {
  const int scale = 2;
  const int char_w = 6 * scale;
  const int line_h = 19;
  uint16_t line = 0;
  uint16_t col = 0;

  for (size_t i = 0; i < ed->len; i++) {
    if (ed->text[i] == '\n') {
      line++;
      col = 0;
      continue;
    }
    if (line >= ed->scroll_line && line < ed->scroll_line + EDITOR_VISIBLE_ROWS) {
      char one[2] = {ed->text[i], 0};
      example_draw_text(ctx, doc_x + col * char_w,
                        doc_y + (line - ed->scroll_line) * line_h,
                        one, example_rgb(31, 39, 52), (uint8_t)scale);
    }
    col++;
    if (col >= EDITOR_WRAP_COLS) {
      line++;
      col = 0;
    }
  }

  visual_pos_t cursor;
  visual_position_at(ed, ed->cursor, &cursor);
  if (((frame / 18u) % 2u) == 0 &&
      cursor.line >= ed->scroll_line &&
      cursor.line < ed->scroll_line + EDITOR_VISIBLE_ROWS) {
    int x = doc_x + cursor.col * char_w;
    int y = doc_y + (cursor.line - ed->scroll_line) * line_h;
    example_fill_rect(ctx, x, y, ed->overwrite ? 12 : 3, 15, example_rgb(31, 39, 52));
  }
}

static void draw_word_processor(example_canvas_t *ctx, const editor_state_t *ed,
                                uint32_t frame) {
  const int page_x = 72;
  const int page_y = 88;
  const int page_w = 656;
  const int page_h = 448;
  const int doc_x = 116;
  const int doc_y = 150;

  example_fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height,
                    example_rgb(32, 38, 46));
  example_fill_rect(ctx, 0, 0, ctx->info.width, 74, example_rgb(33, 96, 122));
  example_draw_text(ctx, 38, 25, "MACHINELAB WRITE", example_rgb(237, 246, 250), 4);

  example_fill_rect(ctx, 392, 18, 84, 34, example_rgb(26, 78, 101));
  example_fill_rect(ctx, 488, 18, 86, 34, ed->dirty ? example_rgb(122, 73, 65)
                                                    : example_rgb(43, 105, 91));
  example_fill_rect(ctx, 586, 18, 128, 34, example_rgb(26, 78, 101));
  example_draw_text(ctx, 408, 31, "DRAFT", example_rgb(222, 238, 243), 2);
  example_draw_text(ctx, 508, 31, ed->dirty ? "DIRTY" : "CLEAN",
                    example_rgb(246, 238, 228), 2);
  example_draw_text(ctx, 603, 31, ed->overwrite ? "OVERWRITE" : "INSERT",
                    example_rgb(222, 238, 243), 2);

  example_fill_rect(ctx, page_x + 8, page_y + 10, page_w, page_h,
                    example_rgb(19, 24, 30));
  example_fill_rect(ctx, page_x, page_y, page_w, page_h, example_rgb(244, 248, 250));
  example_stroke_rect(ctx, page_x, page_y, page_w, page_h, 3, example_rgb(169, 186, 198));
  example_fill_rect(ctx, 94, 124, 612, 2, example_rgb(220, 228, 234));
  example_fill_rect(ctx, 94, 498, 612, 2, example_rgb(220, 228, 234));

  example_draw_text(ctx, 96, 111, "UNTITLED MACHINE NOTE", example_rgb(70, 84, 98), 2);
  for (uint8_t i = 0; i < 10; i++) {
    int x = doc_x + (int)i * 6 * 6 * 2;
    example_fill_rect(ctx, x, 133, 1, 6, example_rgb(196, 207, 216));
  }

  draw_visible_text(ctx, ed, frame, doc_x, doc_y);

  uint16_t total_lines = total_visual_lines(ed);
  if (total_lines > EDITOR_VISIBLE_ROWS) {
    int bar_h = 344;
    int thumb_h = (int)((uint32_t)EDITOR_VISIBLE_ROWS * (uint32_t)bar_h / total_lines);
    if (thumb_h < 28) thumb_h = 28;
    int max_scroll = total_lines - EDITOR_VISIBLE_ROWS;
    int thumb_y = 150 + (max_scroll == 0 ? 0 : ((int)ed->scroll_line * (bar_h - thumb_h)) / max_scroll);
    example_fill_rect(ctx, 696, 150, 6, bar_h, example_rgb(219, 226, 231));
    example_fill_rect(ctx, 694, thumb_y, 10, thumb_h, example_rgb(83, 111, 127));
  }

  visual_pos_t cursor;
  visual_position_at(ed, ed->cursor, &cursor);
  char stats[96];
  snprintf(stats, sizeof(stats), "CHARS %u  WORDS %u  LINES %u  CUR %u:%u",
           (unsigned)ed->len, word_count(ed), total_lines,
           (unsigned)(cursor.line + 1u), (unsigned)(cursor.col + 1u));
  example_fill_rect(ctx, 0, 558, ctx->info.width, 42, example_rgb(24, 29, 35));
  example_draw_text(ctx, 40, 573, stats, example_rgb(205, 214, 222), 2);
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

  example_canvas_t canvas = {fb, info};
  editor_state_t editor = {0};
  uint32_t frame = 0;
  bool running = true;

  draw_word_processor(&canvas, &editor, frame);
  lcom_vbe_present();

  while (running) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) break;

    if ((ev.irq_mask & timer_irq.mask) != 0) {
      frame++;
      draw_word_processor(&canvas, &editor, frame);
      lcom_vbe_present();
    }

    if ((ev.irq_mask & kbd_irq.mask) != 0) {
      uint8_t data = 0;
      if (read_keyboard_byte(&data) == LCOM_OK) {
        handle_keyboard_byte(&editor, data, &running);
        draw_word_processor(&canvas, &editor, frame);
        lcom_vbe_present();
      }
    }
  }

  draw_word_processor(&canvas, &editor, frame);
  lcom_vbe_present();
  lcom_printf("word processor text %s\n", editor.text);

  lcom_irq_unsubscribe(&kbd_irq);
  lcom_irq_unsubscribe(&timer_irq);
  lcom_phys_unmap(fb, info.framebuffer_size);
  lcom_exit();
  return 0;
}
