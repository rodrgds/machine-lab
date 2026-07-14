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

#define MUSIC_STEPS 16u
#define MUSIC_NOTES 12u
#define MUSIC_TRACKS 4u

typedef enum {
  SYNTH_CHIP,
  SYNTH_BASS,
  SYNTH_BELL,
  SYNTH_DRUM,
} synth_kind_t;

typedef struct {
  const char *name;
  const char *subtitle;
  uint32_t color;
  synth_kind_t synth;
} track_def_t;

typedef struct {
  uint8_t notes[MUSIC_TRACKS][MUSIC_STEPS];
  bool muted[MUSIC_TRACKS];
  uint8_t cursor_step;
  uint8_t cursor_row;
  uint8_t track;
  uint8_t play_step;
  uint16_t tempo;
  uint16_t step_phase;
  uint32_t submitted_steps;
  bool playing;
  bool extended;
} sequencer_state_t;

static const char *note_names[MUSIC_NOTES] = {
    "B4", "A#4", "A4", "G#4", "G4", "F#4",
    "F4", "E4", "D#4", "D4", "C#4", "C4",
};

static const uint16_t note_hz[MUSIC_NOTES] = {
    494, 466, 440, 415, 392, 370, 349, 330, 311, 294, 277, 262,
};

static const track_def_t tracks[MUSIC_TRACKS] = {
    {"CHIP", "PULSE LEAD", 0xFFBE55, SYNTH_CHIP},
    {"BASS", "LOW SQUARE", 0x56D5C5, SYNTH_BASS},
    {"BELL", "SOFT PLUCK", 0xA98BFF, SYNTH_BELL},
    {"DRUM", "KIT", 0xFF7D6E, SYNTH_DRUM},
};

static int ac97_start_pcm(uint32_t sample_rate, size_t byte_count) {
  if (byte_count == 0 || byte_count > 0xFFFFu) return LCOM_ERR;
  if (lcom_port_write16(AC97_NAM_BASE + AC97_PCM_FRONT_DAC_RATE,
                        (uint16_t)sample_rate) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_write16(AC97_BM_BASE + AC97_PO_PICB,
                        (uint16_t)byte_count) != LCOM_OK) return LCOM_ERR;
  return lcom_port_write8(AC97_BM_BASE + AC97_PO_CR, AC97_PO_CR_RUN);
}

static void ac97_stop_pcm(void) {
  lcom_port_write8(AC97_BM_BASE + AC97_PO_CR, 0);
}

static void sequencer_seed(sequencer_state_t *st) {
  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    for (uint8_t step = 0; step < MUSIC_STEPS; step++) st->notes[track][step] = 0;
    st->muted[track] = false;
  }

  const uint8_t lead[MUSIC_STEPS] = {12, 0, 10, 0, 8, 0, 7, 0, 8, 0, 5, 0, 7, 0, 10, 0};
  const uint8_t bass[MUSIC_STEPS] = {12, 0, 0, 0, 8, 0, 0, 0, 7, 0, 0, 0, 10, 0, 0, 0};
  const uint8_t bell[MUSIC_STEPS] = {0, 8, 0, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 10, 0, 5};
  const uint8_t drum[MUSIC_STEPS] = {12, 2, 2, 2, 7, 2, 2, 2, 12, 2, 2, 2, 7, 2, 2, 2};
  for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
    st->notes[0][step] = lead[step];
    st->notes[1][step] = bass[step];
    st->notes[2][step] = bell[step];
    st->notes[3][step] = drum[step];
  }

  st->cursor_step = 0;
  st->cursor_row = 11;
  st->track = 0;
  st->play_step = 0;
  st->tempo = 126;
  st->step_phase = 0;
  st->submitted_steps = 0;
  st->playing = false;
  st->extended = false;
}

static uint32_t count_active(const sequencer_state_t *st) {
  uint32_t active = 0;
  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
      if (st->notes[track][step] != 0) active++;
    }
  }
  return active;
}

static uint32_t frames_for_step(uint16_t tempo) {
  uint32_t frames = AC97_DEFAULT_SAMPLE_RATE * 60u / ((uint32_t)tempo * 4u);
  if (frames < 1800u) frames = 1800u;
  if (frames > 12000u) frames = 12000u;
  return frames;
}

static int16_t square_wave(uint32_t frame, uint16_t hz, int32_t amplitude) {
  uint32_t period = AC97_DEFAULT_SAMPLE_RATE / hz;
  if (period < 2u) period = 2u;
  return (frame % period) < period / 2u ? (int16_t)amplitude : (int16_t)-amplitude;
}

static int16_t triangle_wave(uint32_t frame, uint16_t hz, int32_t amplitude) {
  uint32_t period = AC97_DEFAULT_SAMPLE_RATE / hz;
  if (period < 4u) period = 4u;
  uint32_t phase = frame % period;
  int32_t value = phase < period / 2u
                      ? -amplitude + (int32_t)(phase * (uint32_t)(4 * amplitude) / period)
                      : 3 * amplitude - (int32_t)(phase * (uint32_t)(4 * amplitude) / period);
  return (int16_t)value;
}

static int16_t drum_wave(uint32_t frame, uint32_t total, uint8_t note) {
  uint32_t decay = total > frame ? total - frame : 0;
  if (note >= 10u) {
    uint32_t hz = 72u - (frame * 34u / (total == 0 ? 1u : total));
    int32_t amp = (int32_t)(6800u * decay / total);
    return square_wave(frame, (uint16_t)hz, amp);
  }
  uint32_t noise = frame * 1103515245u + 12345u + (uint32_t)note * 977u;
  int32_t sign = ((noise >> 16) & 1u) != 0 ? 1 : -1;
  uint32_t gate = note <= 3u ? total / 4u : total / 2u;
  if (frame >= gate) return 0;
  int32_t amp = note <= 3u ? 2600 : 4600;
  return (int16_t)(sign * amp * (int32_t)(gate - frame) / (int32_t)gate);
}

static int16_t synth_sample(synth_kind_t synth, uint8_t note, uint32_t frame,
                            uint32_t total) {
  if (note == 0 || note > MUSIC_NOTES) return 0;
  uint8_t row = (uint8_t)(note - 1u);
  uint16_t hz = note_hz[row];
  uint32_t gate = total * 7u / 8u;
  if (synth != SYNTH_DRUM && frame >= gate) return 0;
  uint32_t remaining = gate > frame ? gate - frame : 0;
  switch (synth) {
  case SYNTH_CHIP:
    return (int16_t)((int32_t)square_wave(frame, hz, 3100) * (int32_t)remaining /
                     (int32_t)gate);
  case SYNTH_BASS:
    return square_wave(frame, (uint16_t)(hz / 2u), 3900);
  case SYNTH_BELL: {
    int32_t body = triangle_wave(frame, (uint16_t)(hz * 2u), 2500);
    int32_t overtone = triangle_wave(frame, (uint16_t)(hz * 3u), 1100);
    int32_t env = (int32_t)(remaining * remaining / gate);
    return (int16_t)((body + overtone) * env / (int32_t)gate);
  }
  case SYNTH_DRUM:
    return drum_wave(frame, total, note);
  }
  return 0;
}

static size_t render_step_audio(const sequencer_state_t *st, uint8_t step,
                                int16_t *pcm, size_t pcm_bytes) {
  const uint8_t channels = AC97_DEFAULT_CHANNELS;
  uint32_t frames = frames_for_step(st->tempo);
  size_t sample_count = (size_t)frames * channels;
  size_t byte_count = sample_count * sizeof(int16_t);
  if (byte_count > pcm_bytes || byte_count > 0xFFFFu) return 0;
  for (uint32_t frame = 0; frame < frames; frame++) {
    int32_t left = 0;
    int32_t right = 0;
    uint8_t voices = 0;
    for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
      if (st->muted[track] || st->notes[track][step] == 0) continue;
      int32_t sample = synth_sample(tracks[track].synth, st->notes[track][step], frame, frames);
      if (track == 0 || track == 2) {
        left += sample;
        right += sample * 3 / 4;
      } else {
        left += sample * 3 / 4;
        right += sample;
      }
      voices++;
    }
    if (voices > 2) {
      left = left * 2 / 3;
      right = right * 2 / 3;
    }
    if (left > 15000) left = 15000;
    if (left < -15000) left = -15000;
    if (right > 15000) right = 15000;
    if (right < -15000) right = -15000;
    pcm[frame * channels] = (int16_t)left;
    pcm[frame * channels + 1u] = (int16_t)right;
  }
  return byte_count;
}

static int play_current_step(sequencer_state_t *st, int16_t *pcm, size_t pcm_bytes) {
  size_t byte_count = render_step_audio(st, st->play_step, pcm, pcm_bytes);
  if (byte_count == 0) return LCOM_ERR;
  if (ac97_start_pcm(AC97_DEFAULT_SAMPLE_RATE, byte_count) != LCOM_OK) return LCOM_ERR;
  st->submitted_steps++;
  return LCOM_OK;
}

static void toggle_playback(sequencer_state_t *st, int16_t *pcm, size_t pcm_bytes) {
  st->playing = !st->playing;
  st->step_phase = 0;
  if (st->playing) {
    st->play_step = 0;
    play_current_step(st, pcm, pcm_bytes);
  } else {
    ac97_stop_pcm();
  }
}

static void move_cursor_extended(sequencer_state_t *st, uint8_t code) {
  switch (code) {
  case 0x4B: st->cursor_step = (uint8_t)((st->cursor_step + MUSIC_STEPS - 1u) % MUSIC_STEPS); break;
  case 0x4D: st->cursor_step = (uint8_t)((st->cursor_step + 1u) % MUSIC_STEPS); break;
  case 0x48: st->cursor_row = (uint8_t)((st->cursor_row + MUSIC_NOTES - 1u) % MUSIC_NOTES); break;
  case 0x50: st->cursor_row = (uint8_t)((st->cursor_row + 1u) % MUSIC_NOTES); break;
  default: break;
  }
}

static void clear_track(sequencer_state_t *st) {
  for (uint8_t step = 0; step < MUSIC_STEPS; step++) st->notes[st->track][step] = 0;
}

static void handle_keyboard_byte(sequencer_state_t *st, uint8_t data, bool *running,
                                 int16_t *pcm, size_t pcm_bytes) {
  if (data == KBD_TWO_BYTE_PFX) {
    st->extended = true;
    return;
  }
  bool break_code = (data & KBD_BREAK_BIT) != 0;
  uint8_t code = (uint8_t)(data & 0x7Fu);
  if (st->extended) {
    if (!break_code) move_cursor_extended(st, code);
    st->extended = false;
    return;
  }
  if (data == ESC_BREAK) {
    *running = false;
    return;
  }
  if (break_code) return;
  if (code == 0x39) {
    uint8_t note = (uint8_t)(st->cursor_row + 1u);
    st->notes[st->track][st->cursor_step] =
        st->notes[st->track][st->cursor_step] == note ? 0 : note;
  } else if (code == 0x0E) {
    st->notes[st->track][st->cursor_step] = 0;
  } else if (code == 0x0F) {
    st->track = (uint8_t)((st->track + 1u) % MUSIC_TRACKS);
  } else if (code >= 0x02 && code <= 0x05) {
    st->track = (uint8_t)(code - 0x02);
  } else if (code == 0x1C || code == 0x19) {
    toggle_playback(st, pcm, pcm_bytes);
  } else if (code == 0x0C && st->tempo > 70) {
    st->tempo = (uint16_t)(st->tempo - 2u);
  } else if (code == 0x0D && st->tempo < 200) {
    st->tempo = (uint16_t)(st->tempo + 2u);
  } else if (code == 0x32) {
    st->muted[st->track] = !st->muted[st->track];
  } else if (code == 0x2E) {
    clear_track(st);
  } else if (code == 0x13) {
    bool was_playing = st->playing;
    sequencer_seed(st);
    if (was_playing) toggle_playback(st, pcm, pcm_bytes);
  }
}

static void draw_transport(example_canvas_t *ctx, const sequencer_state_t *st) {
  example_fill_rect(ctx, 0, 0, 800, 82, example_rgb(31, 35, 47));
  example_fill_rect(ctx, 0, 80, 800, 2, example_rgb(169, 139, 255));
  example_draw_text(ctx, 28, 20, "ML", example_rgb(178, 148, 255), 4);
  example_draw_text(ctx, 91, 18, "POCKET COMPOSER", example_rgb(241, 241, 247), 3);
  example_draw_text(ctx, 92, 47, "PATTERN 01", example_rgb(139, 147, 166), 1);
  char tempo[24];
  snprintf(tempo, sizeof(tempo), "%u BPM", st->tempo);
  example_fill_rect(ctx, 544, 20, 90, 36, example_rgb(46, 52, 68));
  example_draw_text(ctx, 561, 33, tempo, example_rgb(221, 224, 235), 1);
  example_fill_rect(ctx, 646, 16, 126, 44,
                    st->playing ? example_rgb(55, 139, 110) : example_rgb(78, 69, 96));
  example_draw_text(ctx, 665, 31, st->playing ? "PAUSE  P" : "PLAY   P",
                    example_rgb(245, 244, 248), 2);
}

static void draw_track_panel(example_canvas_t *ctx, const sequencer_state_t *st) {
  example_fill_rect(ctx, 20, 104, 196, 350, example_rgb(30, 35, 46));
  example_stroke_rect(ctx, 20, 104, 196, 350, 1, example_rgb(61, 69, 87));
  example_draw_text(ctx, 36, 120, "INSTRUMENTS", example_rgb(154, 165, 185), 1);
  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    int y = 148 + track * 68;
    uint32_t panel = track == st->track ? example_rgb(55, 61, 78) : example_rgb(38, 44, 57);
    example_fill_rect(ctx, 32, y, 172, 56, panel);
    if (track == st->track) {
      example_stroke_rect(ctx, 32, y, 172, 56, 2, tracks[track].color);
    }
    example_fill_rect(ctx, 44, y + 11, 22, 34, tracks[track].color);
    example_draw_text(ctx, 78, y + 10, tracks[track].name, example_rgb(240, 241, 246), 2);
    example_draw_text(ctx, 78, y + 33, tracks[track].subtitle, example_rgb(153, 164, 182), 1);
    if (st->muted[track]) {
      example_fill_rect(ctx, 168, y + 8, 28, 16, example_rgb(109, 66, 75));
      example_draw_text(ctx, 175, y + 13, "M", example_rgb(247, 227, 231), 1);
    }
  }
  example_draw_text(ctx, 36, 427, "1 2 3 4  TAB", example_rgb(128, 141, 162), 1);
}

static void draw_piano_roll(example_canvas_t *ctx, const sequencer_state_t *st) {
  const int grid_x = 278;
  const int grid_y = 142;
  const int cell_w = 29;
  const int cell_h = 24;
  for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
    char label[3];
    snprintf(label, sizeof(label), "%u", (unsigned)(step + 1u));
    uint32_t color = step == st->play_step && st->playing
                         ? example_rgb(178, 148, 255)
                         : example_rgb(58, 65, 81);
    example_fill_rect(ctx, grid_x + step * cell_w, 112, cell_w - 2, 20, color);
    example_draw_text(ctx, grid_x + step * cell_w + (step < 9 ? 9 : 5), 118,
                      label, example_rgb(230, 232, 238), 1);
  }
  for (uint8_t row = 0; row < MUSIC_NOTES; row++) {
    bool black = note_names[row][1] == '#';
    example_fill_rect(ctx, 230, grid_y + row * cell_h, 42, cell_h - 1,
                      black ? example_rgb(48, 54, 68) : example_rgb(221, 223, 228));
    example_draw_text(ctx, 238, grid_y + row * cell_h + 8, note_names[row],
                      black ? example_rgb(225, 228, 236) : example_rgb(52, 58, 72), 1);
    for (uint8_t step = 0; step < MUSIC_STEPS; step++) {
      int x = grid_x + step * cell_w;
      int y = grid_y + row * cell_h;
      uint32_t base = step % 4u == 0 ? example_rgb(48, 54, 69) : example_rgb(39, 45, 58);
      if (st->playing && step == st->play_step) base = example_rgb(60, 61, 82);
      example_fill_rect(ctx, x, y, cell_w - 2, cell_h - 2, base);
      for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
        if (st->notes[track][step] != row + 1u) continue;
        if (track == st->track) {
          example_fill_rect(ctx, x + 4, y + 5, cell_w - 10, cell_h - 11, tracks[track].color);
        } else {
          example_fill_rect(ctx, x + 4 + track * 5, y + cell_h - 7, 4, 3, tracks[track].color);
        }
      }
    }
  }
  int cursor_x = grid_x + st->cursor_step * cell_w;
  int cursor_y = grid_y + st->cursor_row * cell_h;
  example_stroke_rect(ctx, cursor_x - 2, cursor_y - 2, cell_w + 2, cell_h + 2, 2,
                      example_rgb(247, 246, 250));
}

static void draw_bottom_panel(example_canvas_t *ctx, const sequencer_state_t *st) {
  example_fill_rect(ctx, 20, 474, 760, 80, example_rgb(30, 35, 46));
  example_stroke_rect(ctx, 20, 474, 760, 80, 1, example_rgb(61, 69, 87));
  example_draw_text(ctx, 36, 490, "MIX", example_rgb(153, 164, 183), 1);
  for (uint8_t track = 0; track < MUSIC_TRACKS; track++) {
    int x = 92 + track * 130;
    bool active = st->notes[track][st->play_step] != 0 && !st->muted[track];
    example_draw_text(ctx, x, 489, tracks[track].name, example_rgb(210, 215, 226), 1);
    example_fill_rect(ctx, x, 510, 96, 12, example_rgb(46, 52, 66));
    example_fill_rect(ctx, x, 510, active ? 88 : 18, 12, tracks[track].color);
  }
  char summary[72];
  snprintf(summary, sizeof(summary), "%s  STEP %02u  NOTES %u",
           st->playing ? "PLAYING" : "READY", (unsigned)(st->play_step + 1u),
           (unsigned)count_active(st));
  example_draw_text(ctx, 36, 536, summary, example_rgb(183, 193, 210), 1);
  example_draw_text(ctx, 600, 536, "SPACE DRAW  M MUTE", example_rgb(150, 161, 180), 1);
}

static void draw_music_maker(example_canvas_t *ctx, const sequencer_state_t *st) {
  example_fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height, example_rgb(23, 27, 36));
  draw_transport(ctx, st);
  draw_track_panel(ctx, st);
  draw_piano_roll(ctx, st);
  draw_bottom_panel(ctx, st);
  example_fill_rect(ctx, 0, 574, 800, 26, example_rgb(17, 20, 27));
  example_draw_text(ctx, 24, 583, "ARROWS MOVE   SPACE DRAW   P PLAY   - + TEMPO   C CLEAR",
                    example_rgb(129, 141, 161), 1);
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;
  lcom_vbe_mode_info_t video_info;
  if (lcom_vbe_get_mode_info(LCOM_VBE_MODE_800_600_24, &video_info) != LCOM_OK) return 1;
  if (lcom_vbe_set_mode(LCOM_VBE_MODE_800_600_24) != LCOM_OK) return 1;
  uint8_t *fb = 0;
  if (lcom_phys_map(video_info.framebuffer_phys, video_info.framebuffer_size, (void **)&fb) != LCOM_OK) return 1;
  lcom_ac97_buffer_info_t audio_info;
  if (lcom_ac97_get_buffer(&audio_info) != LCOM_OK) return 1;
  int16_t *pcm = 0;
  if (lcom_phys_map(audio_info.pcm_phys, audio_info.pcm_bytes, (void **)&pcm) != LCOM_OK) return 1;
  lcom_irq_t timer_irq;
  lcom_irq_t kbd_irq;
  if (lcom_irq_subscribe(TIMER0_IRQ, 0, &timer_irq) != LCOM_OK) return 1;
  if (lcom_irq_subscribe(KBC_IRQ, 0, &kbd_irq) != LCOM_OK) return 1;
  sequencer_state_t sequencer;
  sequencer_seed(&sequencer);
  example_canvas_t canvas = {fb, video_info};
  bool running = true;
  draw_music_maker(&canvas, &sequencer);
  lcom_vbe_present();
  while (running) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) break;
    if ((ev.irq_mask & timer_irq.mask) != 0) {
      if (sequencer.playing) {
        sequencer.step_phase = (uint16_t)(sequencer.step_phase + sequencer.tempo * 4u);
        if (sequencer.step_phase >= 3600u) {
          sequencer.step_phase = (uint16_t)(sequencer.step_phase - 3600u);
          sequencer.play_step = (uint8_t)((sequencer.play_step + 1u) % MUSIC_STEPS);
          play_current_step(&sequencer, pcm, audio_info.pcm_bytes);
        }
      }
      draw_music_maker(&canvas, &sequencer);
      lcom_vbe_present();
    }
    if ((ev.irq_mask & kbd_irq.mask) != 0) {
      uint8_t status = 0;
      uint8_t data = 0;
      if (lcom_port_read8(KBC_ST_REG, &status) == LCOM_OK && (status & KBC_ST_OBF) != 0 &&
          (status & KBC_ST_AUX) == 0 && lcom_port_read8(KBC_OUT_BUF, &data) == LCOM_OK) {
        handle_keyboard_byte(&sequencer, data, &running, pcm, audio_info.pcm_bytes);
        draw_music_maker(&canvas, &sequencer);
        lcom_vbe_present();
      }
    }
  }
  ac97_stop_pcm();
  draw_music_maker(&canvas, &sequencer);
  lcom_vbe_present();
  lcom_printf("music maker pattern tracks=%u steps=%u active=%u tempo=%u submitted=%u\n",
              MUSIC_TRACKS, MUSIC_STEPS, count_active(&sequencer), sequencer.tempo,
              (unsigned)sequencer.submitted_steps);
  lcom_phys_unmap(pcm, audio_info.pcm_bytes);
  lcom_irq_unsubscribe(&kbd_irq);
  lcom_irq_unsubscribe(&timer_irq);
  lcom_phys_unmap(fb, video_info.framebuffer_size);
  lcom_exit();
  return 0;
}
