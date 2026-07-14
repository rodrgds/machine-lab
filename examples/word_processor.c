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
#define EDITOR_WRAP_COLS 41u
#define EDITOR_VISIBLE_ROWS 17u

typedef struct {
  char text[EDITOR_MAX_TEXT + 1u];
  char undo_text[EDITOR_MAX_TEXT + 1u];
  size_t len;
  size_t cursor;
  size_t undo_len;
  size_t undo_cursor;
  uint16_t scroll_line;
  bool extended;
  bool overwrite;
  bool dirty;
  bool shift_left;
  bool shift_right;
  bool ctrl;
  bool caps_lock;
  bool help;
  bool have_undo;
  char status[32];
} editor_state_t;

typedef struct {
  uint16_t line;
  uint16_t col;
} visual_pos_t;

static bool is_letter_code(uint8_t code) {
  return (code >= 0x10 && code <= 0x19) || (code >= 0x1E && code <= 0x26) ||
         (code >= 0x2C && code <= 0x32);
}

static char base_scancode_char(uint8_t code) {
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
  case 0x10: return 'q';
  case 0x11: return 'w';
  case 0x12: return 'e';
  case 0x13: return 'r';
  case 0x14: return 't';
  case 0x15: return 'y';
  case 0x16: return 'u';
  case 0x17: return 'i';
  case 0x18: return 'o';
  case 0x19: return 'p';
  case 0x1A: return '[';
  case 0x1B: return ']';
  case 0x1E: return 'a';
  case 0x1F: return 's';
  case 0x20: return 'd';
  case 0x21: return 'f';
  case 0x22: return 'g';
  case 0x23: return 'h';
  case 0x24: return 'j';
  case 0x25: return 'k';
  case 0x26: return 'l';
  case 0x27: return ';';
  case 0x28: return '\'';
  case 0x2B: return '\\';
  case 0x2C: return 'z';
  case 0x2D: return 'x';
  case 0x2E: return 'c';
  case 0x2F: return 'v';
  case 0x30: return 'b';
  case 0x31: return 'n';
  case 0x32: return 'm';
  case 0x33: return ',';
  case 0x34: return '.';
  case 0x35: return '/';
  case 0x39: return ' ';
  default: return 0;
  }
}

static char shifted_symbol(char c) {
  switch (c) {
  case '1': return '!';
  case '-': return '_';
  case '=': return '+';
  case '[': return '{';
  case ']': return '}';
  case ';': return ':';
  case '\'': return '"';
  case '\\': return '|';
  case ',': return '<';
  case '.': return '>';
  case '/': return '?';
  default: return c;
  }
}

static char scancode_to_char(const editor_state_t *ed, uint8_t code) {
  char c = base_scancode_char(code);
  bool shift = ed->shift_left || ed->shift_right;
  if (c == 0 || c == ' ') return c;
  if (is_letter_code(code)) {
    bool upper = shift != ed->caps_lock;
    return upper ? (char)(c - 'a' + 'A') : c;
  }
  return shift ? shifted_symbol(c) : c;
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
    } else if (++col >= EDITOR_WRAP_COLS) {
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
    } else if (++col >= EDITOR_WRAP_COLS) {
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
    if (space) in_word = false;
    else if (!in_word) {
      words++;
      in_word = true;
    }
  }
  return words;
}

static void set_status(editor_state_t *ed, const char *status) {
  snprintf(ed->status, sizeof(ed->status), "%s", status);
}

static void ensure_cursor_visible(editor_state_t *ed) {
  visual_pos_t pos;
  visual_position_at(ed, ed->cursor, &pos);
  if (pos.line < ed->scroll_line) ed->scroll_line = pos.line;
  else if (pos.line >= ed->scroll_line + EDITOR_VISIBLE_ROWS) {
    ed->scroll_line = (uint16_t)(pos.line - EDITOR_VISIBLE_ROWS + 1u);
  }
}

static void remember_undo(editor_state_t *ed) {
  memcpy(ed->undo_text, ed->text, ed->len + 1u);
  ed->undo_len = ed->len;
  ed->undo_cursor = ed->cursor;
  ed->have_undo = true;
}

static void undo(editor_state_t *ed) {
  if (!ed->have_undo) {
    set_status(ed, "NOTHING TO UNDO");
    return;
  }
  char current[EDITOR_MAX_TEXT + 1u];
  size_t current_len = ed->len;
  size_t current_cursor = ed->cursor;
  memcpy(current, ed->text, ed->len + 1u);
  memcpy(ed->text, ed->undo_text, ed->undo_len + 1u);
  ed->len = ed->undo_len;
  ed->cursor = ed->undo_cursor;
  memcpy(ed->undo_text, current, current_len + 1u);
  ed->undo_len = current_len;
  ed->undo_cursor = current_cursor;
  ed->dirty = true;
  set_status(ed, "UNDO");
  ensure_cursor_visible(ed);
}

static void insert_char(editor_state_t *ed, char c) {
  if (ed->len >= EDITOR_MAX_TEXT) {
    set_status(ed, "DOCUMENT FULL");
    return;
  }
  remember_undo(ed);
  if (ed->overwrite && ed->cursor < ed->len && ed->text[ed->cursor] != '\n') {
    ed->text[ed->cursor++] = c;
  } else {
    memmove(&ed->text[ed->cursor + 1u], &ed->text[ed->cursor], ed->len - ed->cursor + 1u);
    ed->text[ed->cursor++] = c;
    ed->len++;
  }
  ed->dirty = true;
  set_status(ed, "EDITING");
  ensure_cursor_visible(ed);
}

static void delete_at_cursor(editor_state_t *ed) {
  if (ed->cursor >= ed->len) return;
  remember_undo(ed);
  memmove(&ed->text[ed->cursor], &ed->text[ed->cursor + 1u], ed->len - ed->cursor);
  ed->len--;
  ed->dirty = true;
  set_status(ed, "EDITING");
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
  case 0x47: move_cursor_home(ed); break;
  case 0x4F: move_cursor_end(ed); break;
  case 0x48: move_cursor_line(ed, -1); break;
  case 0x50: move_cursor_line(ed, 1); break;
  case 0x4B:
    if (ed->cursor > 0) ed->cursor--;
    ensure_cursor_visible(ed);
    break;
  case 0x4D:
    if (ed->cursor < ed->len) ed->cursor++;
    ensure_cursor_visible(ed);
    break;
  case 0x49:
    ed->scroll_line = ed->scroll_line > EDITOR_VISIBLE_ROWS
                          ? (uint16_t)(ed->scroll_line - EDITOR_VISIBLE_ROWS)
                          : 0;
    break;
  case 0x51:
    ed->scroll_line = (uint16_t)(ed->scroll_line + EDITOR_VISIBLE_ROWS);
    if (ed->scroll_line >= total_visual_lines(ed)) ed->scroll_line = 0;
    break;
  case 0x52:
    ed->overwrite = !ed->overwrite;
    set_status(ed, ed->overwrite ? "OVERWRITE MODE" : "INSERT MODE");
    break;
  case 0x53: delete_at_cursor(ed); break;
  default: break;
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
  if (code == 0x2A) {
    ed->shift_left = !break_code;
    return;
  }
  if (code == 0x36) {
    ed->shift_right = !break_code;
    return;
  }
  if (code == 0x1D) {
    ed->ctrl = !break_code;
    return;
  }
  if (data == ESC_BREAK) {
    *running = false;
    return;
  }
  if (break_code) return;
  if (code == 0x3A) {
    ed->caps_lock = !ed->caps_lock;
    set_status(ed, ed->caps_lock ? "CAPS LOCK ON" : "CAPS LOCK OFF");
  } else if (code == 0x3B) {
    ed->help = !ed->help;
  } else if (code == 0x3C || (ed->ctrl && code == 0x1F)) {
    ed->dirty = false;
    set_status(ed, "SAVED LOCALLY");
  } else if (ed->ctrl && code == 0x2C) {
    undo(ed);
  } else if (code == 0x0E) {
    backspace(ed);
  } else if (code == 0x1C) {
    insert_char(ed, '\n');
  } else if (code == 0x0F) {
    for (uint8_t i = 0; i < 4; i++) insert_char(ed, ' ');
  } else {
    char c = scancode_to_char(ed, code);
    if (c != 0) insert_char(ed, c);
  }
}

static void draw_visible_text(example_canvas_t *ctx, const editor_state_t *ed, uint32_t frame,
                              int doc_x, int doc_y) {
  const int char_w = 12;
  const int line_h = 20;
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
                        one, example_rgb(39, 45, 57), 2);
    }
    if (++col >= EDITOR_WRAP_COLS) {
      line++;
      col = 0;
    }
  }
  visual_pos_t cursor;
  visual_position_at(ed, ed->cursor, &cursor);
  if (((frame / 18u) % 2u) == 0 && cursor.line >= ed->scroll_line &&
      cursor.line < ed->scroll_line + EDITOR_VISIBLE_ROWS) {
    int x = doc_x + cursor.col * char_w;
    int y = doc_y + (cursor.line - ed->scroll_line) * line_h;
    example_fill_rect(ctx, x, y, ed->overwrite ? 11 : 2, 16, example_rgb(58, 104, 181));
  }
}

static void draw_sidebar(example_canvas_t *ctx) {
  example_fill_rect(ctx, 0, 70, 64, 488, example_rgb(31, 36, 48));
  const char *items[] = {"DOC", "FIND", "INFO"};
  for (uint8_t i = 0; i < 3; i++) {
    int y = 104 + i * 70;
    if (i == 0) example_fill_rect(ctx, 0, y - 12, 4, 48, example_rgb(103, 144, 226));
    example_draw_text(ctx, 12, y, items[i],
                      i == 0 ? example_rgb(236, 240, 248) : example_rgb(128, 139, 158), 1);
  }
}

static void draw_inspector(example_canvas_t *ctx, const editor_state_t *ed) {
  example_fill_rect(ctx, 664, 104, 116, 390, example_rgb(34, 40, 51));
  example_stroke_rect(ctx, 664, 104, 116, 390, 1, example_rgb(60, 70, 86));
  example_draw_text(ctx, 680, 124, "DOCUMENT", example_rgb(185, 195, 212), 1);
  example_draw_text(ctx, 680, 154, "MACHINE", example_rgb(239, 241, 246), 2);
  example_draw_text(ctx, 680, 174, "NOTES", example_rgb(239, 241, 246), 2);
  example_fill_rect(ctx, 680, 210, 84, 1, example_rgb(69, 79, 95));
  example_draw_text(ctx, 680, 232, "MODE", example_rgb(132, 145, 164), 1);
  example_draw_text(ctx, 680, 250, ed->overwrite ? "OVERWRITE" : "INSERT",
                    example_rgb(116, 158, 235), 1);
  example_draw_text(ctx, 680, 286, "CASE", example_rgb(132, 145, 164), 1);
  example_draw_text(ctx, 680, 304, ed->caps_lock ? "CAPS" : "NORMAL",
                    example_rgb(224, 185, 102), 1);
  example_draw_text(ctx, 680, 346, "SHORTCUTS", example_rgb(132, 145, 164), 1);
  example_draw_text(ctx, 680, 368, "CTRL S", example_rgb(208, 214, 225), 1);
  example_draw_text(ctx, 680, 388, "CTRL Z", example_rgb(208, 214, 225), 1);
  example_draw_text(ctx, 680, 408, "F1 HELP", example_rgb(208, 214, 225), 1);
  example_draw_text(ctx, 680, 428, "F2 SAVE", example_rgb(208, 214, 225), 1);
}

static void draw_help(example_canvas_t *ctx) {
  example_fill_rect(ctx, 136, 142, 528, 300, example_rgb(26, 31, 42));
  example_stroke_rect(ctx, 136, 142, 528, 300, 3, example_rgb(103, 144, 226));
  example_draw_text(ctx, 172, 176, "KEYBOARD FIRST EDITOR", example_rgb(239, 242, 248), 3);
  example_draw_text(ctx, 172, 228, "ARROWS     MOVE CURSOR", example_rgb(194, 204, 220), 2);
  example_draw_text(ctx, 172, 258, "HOME END   MOVE IN LINE", example_rgb(194, 204, 220), 2);
  example_draw_text(ctx, 172, 288, "INSERT     CHANGE MODE", example_rgb(194, 204, 220), 2);
  example_draw_text(ctx, 172, 318, "CTRL Z     UNDO REDO", example_rgb(194, 204, 220), 2);
  example_draw_text(ctx, 172, 348, "CTRL S     SAVE", example_rgb(194, 204, 220), 2);
  example_draw_text(ctx, 172, 390, "F1 TO CLOSE", example_rgb(116, 158, 235), 2);
}

static void draw_word_processor(example_canvas_t *ctx, const editor_state_t *ed,
                                uint32_t frame) {
  const int page_x = 88;
  const int page_y = 112;
  const int page_w = 556;
  const int page_h = 402;
  const int doc_x = 134;
  const int doc_y = 158;
  example_fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height, example_rgb(24, 28, 37));
  example_fill_rect(ctx, 0, 0, ctx->info.width, 70, example_rgb(37, 44, 58));
  example_fill_rect(ctx, 0, 68, ctx->info.width, 2, example_rgb(103, 144, 226));
  example_draw_text(ctx, 28, 21, "ML", example_rgb(119, 157, 232), 4);
  example_draw_text(ctx, 92, 19, "MACHINE NOTES", example_rgb(240, 243, 248), 3);
  example_draw_text(ctx, 92, 47, "UNTITLED.MLTXT", example_rgb(139, 151, 170), 1);
  example_fill_rect(ctx, 570, 18, 88, 32,
                    ed->dirty ? example_rgb(109, 67, 72) : example_rgb(48, 104, 83));
  example_draw_text(ctx, 591, 30, ed->dirty ? "UNSAVED" : "SAVED",
                    example_rgb(242, 239, 236), 1);
  example_fill_rect(ctx, 670, 18, 100, 32, example_rgb(48, 58, 75));
  example_draw_text(ctx, 690, 30, ed->overwrite ? "OVERWRITE" : "INSERT",
                    example_rgb(210, 218, 231), 1);
  draw_sidebar(ctx);
  example_fill_rect(ctx, page_x + 7, page_y + 8, page_w, page_h, example_rgb(13, 16, 22));
  example_fill_rect(ctx, page_x, page_y, page_w, page_h, example_rgb(247, 246, 241));
  example_stroke_rect(ctx, page_x, page_y, page_w, page_h, 2, example_rgb(181, 187, 195));
  example_fill_rect(ctx, 112, 140, 508, 1, example_rgb(218, 217, 211));
  for (uint8_t row = 0; row < EDITOR_VISIBLE_ROWS; row++) {
    char line[4];
    snprintf(line, sizeof(line), "%u", (unsigned)(ed->scroll_line + row + 1u));
    example_draw_text(ctx, 102, doc_y + row * 20 + 4, line, example_rgb(170, 173, 171), 1);
  }
  example_fill_rect(ctx, 124, 150, 1, 344, example_rgb(225, 224, 219));
  draw_visible_text(ctx, ed, frame, doc_x, doc_y);
  draw_inspector(ctx, ed);
  uint16_t total_lines = total_visual_lines(ed);
  if (total_lines > EDITOR_VISIBLE_ROWS) {
    int bar_h = 336;
    int thumb_h = (int)((uint32_t)EDITOR_VISIBLE_ROWS * (uint32_t)bar_h / total_lines);
    if (thumb_h < 28) thumb_h = 28;
    int max_scroll = total_lines - EDITOR_VISIBLE_ROWS;
    int thumb_y = 154 + (max_scroll == 0 ? 0 : ((int)ed->scroll_line * (bar_h - thumb_h)) / max_scroll);
    example_fill_rect(ctx, 632, 154, 4, bar_h, example_rgb(220, 219, 214));
    example_fill_rect(ctx, 630, thumb_y, 8, thumb_h, example_rgb(112, 126, 148));
  }
  visual_pos_t cursor;
  visual_position_at(ed, ed->cursor, &cursor);
  char stats[96];
  snprintf(stats, sizeof(stats), "LN %u  COL %u    WORDS %u    CHARS %u",
           (unsigned)(cursor.line + 1u), (unsigned)(cursor.col + 1u),
           word_count(ed), (unsigned)ed->len);
  example_fill_rect(ctx, 0, 558, ctx->info.width, 42, example_rgb(18, 21, 28));
  example_draw_text(ctx, 26, 573, stats, example_rgb(183, 194, 211), 2);
  example_draw_text(ctx, 612, 573, ed->status, example_rgb(116, 158, 235), 1);
  if (ed->help) draw_help(ctx);
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;
  lcom_vbe_mode_info_t info;
  if (lcom_vbe_get_mode_info(LCOM_VBE_MODE_800_600_24, &info) != LCOM_OK) return 1;
  if (lcom_vbe_set_mode(LCOM_VBE_MODE_800_600_24) != LCOM_OK) return 1;
  uint8_t *fb = 0;
  if (lcom_phys_map(info.framebuffer_phys, info.framebuffer_size, (void **)&fb) != LCOM_OK) return 1;
  lcom_irq_t timer_irq;
  lcom_irq_t kbd_irq;
  if (lcom_irq_subscribe(TIMER0_IRQ, 0, &timer_irq) != LCOM_OK) return 1;
  if (lcom_irq_subscribe(KBC_IRQ, 0, &kbd_irq) != LCOM_OK) return 1;
  example_canvas_t canvas = {fb, info};
  editor_state_t editor = {0};
  set_status(&editor, "READY");
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
