#include "example_ui.h"

#include <lcom/ac97.h>
#include <lcom/i8042.h>
#include <lcom/i8254.h>
#include <lcom/lcom.h>
#include <lcom/vbe.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MUSIC_STEPS 16
#define MUSIC_NOTES 8
#define MUSIC_TRACKS 3

typedef struct {
  const char *name;
  uint32_t color;
} track_def_t;

typedef struct {
  uint8_t pattern[MUSIC_TRACKS][MUSIC_NOTES][MUSIC_STEPS];
  uint8_t cursor_step;
  uint8_t cursor_row;
  uint8_t track;
  uint8_t play_step;
  uint16_t tempo;
  uint16_t frames_in_step;
  bool playing;
  bool extended;
} tracker_state_t;

static const char *note_names[MUSIC_NOTES] = {"C5", "B4", "A4", "G4", "F4", "E4", "D4", "C4"};
static const uint16_t note_hz[MUSIC_NOTES] = {523, 494, 440, 392, 349, 330, 294, 262};
static const track_def_t tracks[MUSIC_TRACKS] = {
    {"LEAD", 0xF4BA4E},
    {"BASS", 0x75D3C4},
    {"DRUM", 0xE8775B},
};

static int ac97_start_pcm(uint32_t sample_rate, size_t byte_count) {
  if (byte_count == 0 || byte_count > 0xFFFFu) return LCOM_ERR;
  if (lcom_port_write16(AC97_NAM_BASE + AC97_PCM_FRONT_DAC_RATE,
                        (uint16_t)sample_rate) != LCOM_OK) {
    return LCOM_ERR;
  }
  if (lcom_port_write16(AC97_BM_BASE + AC97_PO_PICB,
                        (uint16_t)byte_count) != LCOM_OK) {
    return LCOM_ERR;
  }
  return lcom_port_write8(AC97_BM_BASE + AC97_PO_CR, AC97_PO_CR_RUN);
}

static void tracker_reset(tracker_state_t *st) {
  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
      for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
        st->pattern[track][row][step] = 0;
      }
    }
  }

  st->pattern[0][0][0] = 1;
  st->pattern[0][2][2] = 1;
  st->pattern[0][4][4] = 1;
  st->pattern[0][5][6] = 1;
  st->pattern[0][3][8] = 1;
  st->pattern[0][1][10] = 1;
  st->pattern[0][4][12] = 1;
  st->pattern[0][6][14] = 1;

  st->pattern[1][7][0] = 1;
  st->pattern[1][7][4] = 1;
  st->pattern[1][5][8] = 1;
  st->pattern[1][6][12] = 1;

  st->pattern[2][7][0] = 1;
  st->pattern[2][2][2] = 1;
  st->pattern[2][7][4] = 1;
  st->pattern[2][4][6] = 1;
  st->pattern[2][7][8] = 1;
  st->pattern[2][2][10] = 1;
  st->pattern[2][7][12] = 1;
  st->pattern[2][4][14] = 1;

  st->cursor_step = 0;
  st->cursor_row = 0;
  st->track = 0;
  st->play_step = 0;
  st->tempo = 124;
  st->frames_in_step = 0;
  st->playing = true;
  st->extended = false;
}

static uint32_t count_active(const tracker_state_t *st) {
  uint32_t active = 0;
  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
      for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
        if (st->pattern[track][row][step] != 0) active++;
      }
    }
  }
  return active;
}

static uint8_t active_at_step(const tracker_state_t *st, uint8_t track, uint8_t step) {
  uint8_t active = 0;
  for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
    if (st->pattern[track][row][step] != 0) active++;
  }
  return active;
}

static uint8_t frames_per_step(uint16_t tempo) {
  uint16_t frames = (uint16_t)(3600u / (tempo * 4u));
  if (frames < 4) frames = 4;
  if (frames > 20) frames = 20;
  return (uint8_t)frames;
}

static void move_cursor_extended(tracker_state_t *st, uint8_t code) {
  switch (code) {
  case 0x4B:
    st->cursor_step = (uint8_t)((st->cursor_step + MUSIC_STEPS - 1u) % MUSIC_STEPS);
    break;
  case 0x4D:
    st->cursor_step = (uint8_t)((st->cursor_step + 1u) % MUSIC_STEPS);
    break;
  case 0x48:
    st->cursor_row = (uint8_t)((st->cursor_row + MUSIC_NOTES - 1u) % MUSIC_NOTES);
    break;
  case 0x50:
    st->cursor_row = (uint8_t)((st->cursor_row + 1u) % MUSIC_NOTES);
    break;
  default:
    break;
  }
}

static bool handle_keyboard_byte(tracker_state_t *st, uint8_t data, bool *running) {
  if (data == KBD_TWO_BYTE_PFX) {
    st->extended = true;
    return false;
  }

  bool break_code = (data & KBD_BREAK_BIT) != 0;
  uint8_t code = (uint8_t)(data & 0x7Fu);

  if (st->extended) {
    if (!break_code) move_cursor_extended(st, code);
    st->extended = false;
    return false;
  }

  if (data == ESC_BREAK) {
    *running = false;
    return false;
  }
  if (break_code) return false;

  if (code == 0x39) {
    uint8_t *cell = &st->pattern[st->track][st->cursor_row][st->cursor_step];
    *cell = (uint8_t)!*cell;
    return true;
  }
  if (code == 0x0F) {
    st->track = (uint8_t)((st->track + 1u) % MUSIC_TRACKS);
  } else if (code >= 0x02 && code <= 0x04) {
    st->track = (uint8_t)(code - 0x02);
  } else if (code == 0x1C || code == 0x19) {
    st->playing = !st->playing;
  } else if (code == 0x0C && st->tempo > 60) {
    st->tempo = (uint16_t)(st->tempo - 4u);
  } else if (code == 0x0D && st->tempo < 220) {
    st->tempo = (uint16_t)(st->tempo + 4u);
  } else if (code == 0x2E) {
    for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
      for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
        st->pattern[st->track][row][step] = 0;
      }
    }
    return true;
  } else if (code == 0x0E) {
    for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
      st->pattern[st->track][row][st->cursor_step] = 0;
    }
    return true;
  }
  return false;
}

static int16_t square_sample(uint32_t frame, uint32_t sample_rate, uint16_t hz, int16_t amp) {
  uint32_t period = sample_rate / hz;
  if (period == 0) period = 1;
  return (frame % period) < (period / 2u) ? amp : (int16_t)-amp;
}

static int16_t drum_sample(uint32_t frame, uint32_t local, uint8_t row) {
  if (local > 130u) return 0;
  int32_t env = 130 - (int32_t)local;
  uint32_t noise = frame * 1103515245u + 12345u + (uint32_t)row * 977u;
  int32_t sign = ((noise >> 16) & 1u) ? 1 : -1;
  int32_t amp = row >= 6 ? 5400 : (row >= 3 ? 3800 : 2600);
  return (int16_t)(sign * amp * env / 130);
}

static size_t render_pattern_audio(const tracker_state_t *st, int16_t *pcm, size_t pcm_bytes) {
  const uint32_t sample_rate = AC97_DEFAULT_SAMPLE_RATE;
  const uint8_t channels = AC97_DEFAULT_CHANNELS;
  const uint32_t step_frames = sample_rate / 96u;
  const uint32_t frames = step_frames * MUSIC_STEPS;
  size_t sample_count = (size_t)frames * channels;
  if (sample_count * sizeof(int16_t) > pcm_bytes || sample_count * sizeof(int16_t) > 0xFFFFu) {
    return 0;
  }

  for (uint32_t frame = 0; frame < frames; frame++) {
    uint8_t step = (uint8_t)(frame / step_frames);
    uint32_t local = frame % step_frames;
    int32_t mix = 0;
    uint8_t voices = 0;
    for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
      for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
        if (st->pattern[track][row][step] == 0) continue;
        if (track == 0) {
          mix += square_sample(frame, sample_rate, note_hz[row], 2400);
        } else if (track == 1) {
          mix += square_sample(frame, sample_rate, (uint16_t)(note_hz[row] / 2u), 3600);
        } else {
          mix += drum_sample(frame, local, row);
        }
        voices++;
      }
    }
    if (voices > 2) mix /= 2;
    if (mix > 14000) mix = 14000;
    if (mix < -14000) mix = -14000;
    pcm[frame * channels + 0] = (int16_t)mix;
    pcm[frame * channels + 1] = (int16_t)mix;
  }
  return sample_count * sizeof(int16_t);
}

static int refresh_audio(const tracker_state_t *st, int16_t *pcm, size_t pcm_bytes) {
  size_t byte_count = render_pattern_audio(st, pcm, pcm_bytes);
  return ac97_start_pcm(AC97_DEFAULT_SAMPLE_RATE, byte_count);
}

static void draw_track_panel(example_canvas_t *ctx, const tracker_state_t *st) {
  example_fill_rect(ctx, 26, 104, 150, 302, example_rgb(36, 43, 52));
  example_stroke_rect(ctx, 26, 104, 150, 302, 2, example_rgb(74, 88, 101));
  example_draw_text(ctx, 48, 122, "TRACKS", example_rgb(214, 222, 228), 2);

  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    int y = 156 + track * 72;
    uint32_t bg = track == st->track ? example_rgb(62, 72, 84) : example_rgb(44, 52, 62);
    example_fill_rect(ctx, 42, y, 116, 52, bg);
    example_fill_rect(ctx, 50, y + 10, 18, 18, tracks[track].color);
    example_draw_text(ctx, 78, y + 11, tracks[track].name, example_rgb(236, 240, 242), 2);
    char count[16];
    snprintf(count, sizeof(count), "%u ON", active_at_step(st, track, st->play_step));
    example_draw_text(ctx, 78, y + 31, count, example_rgb(185, 198, 207), 1);
  }

  char tempo[24];
  snprintf(tempo, sizeof(tempo), "%u BPM", st->tempo);
  example_fill_rect(ctx, 42, 374, 116, 22, example_rgb(27, 33, 40));
  example_draw_text(ctx, 54, 382, tempo, example_rgb(217, 226, 232), 1);
}

static void draw_grid(example_canvas_t *ctx, const tracker_state_t *st) {
  const int grid_x = 236;
  const int grid_y = 128;
  const int cell_w = 30;
  const int cell_h = 35;

  for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
    char step_label[3];
    snprintf(step_label, sizeof(step_label), "%u", (unsigned)(step + 1u));
    uint32_t label_bg = step == st->play_step ? example_rgb(81, 92, 105)
                                              : example_rgb(45, 52, 61);
    example_fill_rect(ctx, grid_x + step * cell_w, grid_y - 25, cell_w - 3, 17, label_bg);
    example_draw_text(ctx, grid_x + step * cell_w + (step < 9 ? 10 : 6),
                      grid_y - 20, step_label, example_rgb(213, 222, 230), 1);
  }

  for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
    example_draw_text(ctx, 188, grid_y + row * cell_h + 12, note_names[row],
                      example_rgb(214, 222, 228), 2);
    for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
      int x = grid_x + step * cell_w;
      int y = grid_y + row * cell_h;
      uint32_t base = ((step + row) % 2u) == 0 ? example_rgb(44, 51, 60)
                                               : example_rgb(38, 45, 54);
      if (step == st->play_step) base = example_rgb(54, 63, 75);
      example_fill_rect(ctx, x, y, cell_w - 3, cell_h - 3, base);

      for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
        if (st->pattern[track][row][step] == 0) continue;
        if (track == st->track) {
          example_fill_rect(ctx, x + 6, y + 8, cell_w - 15, cell_h - 18,
                            tracks[track].color);
        } else {
          example_fill_rect(ctx, x + 9 + track * 4, y + 24, 4, 4, tracks[track].color);
        }
      }
    }
  }

  int cursor_x = grid_x + st->cursor_step * cell_w;
  int cursor_y = grid_y + st->cursor_row * cell_h;
  example_stroke_rect(ctx, cursor_x - 2, cursor_y - 2, cell_w + 1, cell_h + 1, 3,
                      example_rgb(235, 241, 243));
}

static void draw_mixer(example_canvas_t *ctx, const tracker_state_t *st) {
  example_fill_rect(ctx, 214, 444, 484, 86, example_rgb(31, 37, 45));
  example_stroke_rect(ctx, 214, 444, 484, 86, 2, example_rgb(70, 82, 95));
  example_draw_text(ctx, 234, 462, "MIXER", example_rgb(208, 218, 226), 2);

  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    uint8_t level = active_at_step(st, track, st->play_step);
    int x = 334 + track * 106;
    int level_w = 12 + level * 8;
    if (level_w > 80) level_w = 80;
    example_draw_text(ctx, x, 462, tracks[track].name, example_rgb(220, 228, 234), 1);
    example_fill_rect(ctx, x, 482, 80, 16, example_rgb(43, 50, 60));
    example_fill_rect(ctx, x, 482, level_w, 16, tracks[track].color);
  }

  char summary[64];
  snprintf(summary, sizeof(summary), "%s  ACTIVE %u",
           st->playing ? "PLAY" : "PAUSE", (unsigned)count_active(st));
  example_draw_text(ctx, 234, 506, summary, example_rgb(187, 199, 209), 1);
}

static void draw_music_maker(example_canvas_t *ctx, const tracker_state_t *st, uint32_t frame) {
  (void)frame;
  example_fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height,
                    example_rgb(24, 29, 36));
  example_fill_rect(ctx, 0, 0, ctx->info.width, 82, example_rgb(88, 65, 118));
  example_draw_text(ctx, 42, 27, "MACHINELAB TRACKER", example_rgb(247, 244, 236), 4);
  example_draw_text(ctx, 534, 28, "PATTERN A", example_rgb(226, 220, 236), 2);
  example_fill_rect(ctx, 686, 22, 72, 34,
                    st->playing ? example_rgb(57, 120, 98) : example_rgb(97, 73, 63));
  example_draw_text(ctx, 704, 34, st->playing ? "PLAY" : "PAUSE",
                    example_rgb(242, 244, 236), 1);

  draw_track_panel(ctx, st);
  draw_grid(ctx, st);
  draw_mixer(ctx, st);
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;

  lcom_vbe_mode_info_t video_info;
  if (lcom_vbe_get_mode_info(LCOM_VBE_MODE_800_600_24, &video_info) != LCOM_OK) return 1;
  if (lcom_vbe_set_mode(LCOM_VBE_MODE_800_600_24) != LCOM_OK) return 1;

  uint8_t *fb = 0;
  if (lcom_phys_map(video_info.framebuffer_phys, video_info.framebuffer_size, (void **)&fb) !=
      LCOM_OK) {
    return 1;
  }

  lcom_ac97_buffer_info_t audio_info;
  if (lcom_ac97_get_buffer(&audio_info) != LCOM_OK) return 1;
  int16_t *pcm = 0;
  if (lcom_phys_map(audio_info.pcm_phys, audio_info.pcm_bytes, (void **)&pcm) != LCOM_OK) {
    return 1;
  }

  lcom_irq_t timer_irq;
  lcom_irq_t kbd_irq;
  if (lcom_irq_subscribe(TIMER0_IRQ, 0, &timer_irq) != LCOM_OK) return 1;
  if (lcom_irq_subscribe(KBC_IRQ, 0, &kbd_irq) != LCOM_OK) return 1;

  tracker_state_t tracker;
  tracker_reset(&tracker);
  refresh_audio(&tracker, pcm, audio_info.pcm_bytes);

  example_canvas_t canvas = {fb, video_info};
  uint32_t frame = 0;
  bool running = true;

  draw_music_maker(&canvas, &tracker, frame);
  lcom_vbe_present();

  while (running) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) break;

    if ((ev.irq_mask & timer_irq.mask) != 0) {
      frame++;
      if (tracker.playing) {
        tracker.frames_in_step++;
        if (tracker.frames_in_step >= frames_per_step(tracker.tempo)) {
          tracker.frames_in_step = 0;
          tracker.play_step = (uint8_t)((tracker.play_step + 1u) % MUSIC_STEPS);
          if (tracker.play_step == 0) refresh_audio(&tracker, pcm, audio_info.pcm_bytes);
        }
      }
      draw_music_maker(&canvas, &tracker, frame);
      lcom_vbe_present();
    }

    if ((ev.irq_mask & kbd_irq.mask) != 0) {
      uint8_t status = 0;
      uint8_t data = 0;
      if (lcom_port_read8(KBC_ST_REG, &status) == LCOM_OK &&
          (status & KBC_ST_OBF) != 0 &&
          (status & KBC_ST_AUX) == 0 &&
          lcom_port_read8(KBC_OUT_BUF, &data) == LCOM_OK) {
        bool audio_changed = handle_keyboard_byte(&tracker, data, &running);
        if (audio_changed) refresh_audio(&tracker, pcm, audio_info.pcm_bytes);
        draw_music_maker(&canvas, &tracker, frame);
        lcom_vbe_present();
      }
    }
  }

  refresh_audio(&tracker, pcm, audio_info.pcm_bytes);
  draw_music_maker(&canvas, &tracker, frame);
  lcom_vbe_present();
  lcom_printf("music maker pattern tracks=%u steps=%u active=%u tempo=%u\n",
              MUSIC_TRACKS, MUSIC_STEPS, count_active(&tracker), tracker.tempo);

  lcom_phys_unmap(pcm, audio_info.pcm_bytes);
  lcom_irq_unsubscribe(&kbd_irq);
  lcom_irq_unsubscribe(&timer_irq);
  lcom_phys_unmap(fb, video_info.framebuffer_size);
  lcom_exit();
  return 0;
}
