/**
 * @file app.c
 * @brief Application controller -- initialisation, interrupt dispatch, and state machine.
 * @ingroup group_app
 *
 * This is the central hub of the game. It initialises VBE graphics,
 * subscribes to hardware interrupts (timer/keyboard/mouse), loads
 * resources, and enters the main loop. Each interrupt is normalised
 * into an app_event_t and dispatched to the active screen handler.
 *
 * Key responsibilities:
 * - Hardware initialisation and cleanup
 * - Interrupt-to-event translation (keyboard, mouse, timer)
 * - Screen state transitions (menu <-> gameplay <-> pause <-> result)
 * - FPS calculation and profiling integration
 */

#include "app.h"

#include "ninjix_platform.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "timer.h"
#include "kbc.h"
#include "kbd.h"
#include "mouse.h"
#include "mouse_packet.h"
#include "video_gr_ext.h"

#include "app_constants.h"
#include "app_events.h"
#include "app_state.h"
#include "scancodes.h"
#include "layout.h"
#include "levels.h"
#include "multiplayer.h"
#include "profiler.h"
#include "resources.h"
#include "serial_port.h"

#define APP_VIDEO_MODE 0x115

typedef enum {
  RESULT_OPTION_RESTART = 0,
  RESULT_OPTION_NEXT = 1,
  RESULT_OPTION_CLOSE = 2,
} result_option_t;

typedef struct {
  app_screen_tag_t tag;
  app_screen_state_t screen;
  resources_t resources;
  uint8_t keyboard_bit_no;
  uint8_t timer_bit_no;
  uint8_t mouse_bit_no;
  int serial_bit_no;
  bool graphics_ready;
  bool timer_ready;
  bool keyboard_ready;
  bool mouse_ready;
  bool serial_ready;
  bool mouse_reporting;
  multiplayer_t multiplayer;
  uint16_t multiplayer_level_id;
  multiplayer_role_t pair_auto_role;
  uint32_t pair_auto_exit_tick;
  uint32_t physics_ticks;
  uint32_t render_ticks;
  uint32_t last_physics_tick;
  uint32_t last_render_tick;
  uint32_t fps_window_start_tick;
  uint32_t frames_in_window;
} app_t;

static int app_handle_event(app_t *app, const app_event_t *event);

static void app_advance_render_ticks(app_t *app, uint32_t current_tick)
{
  app->last_render_tick += app->render_ticks;
  if ((current_tick - app->last_render_tick) >= app->render_ticks)
    app->last_render_tick = current_tick;
}

static menu_t *app_menu(app_t *app)
{
  return &app->screen.menu;
}

static scene_t *app_scene(app_t *app)
{
  return &app->screen.gameplay.scene;
}

static pause_menu_t *app_pause(app_t *app)
{
  return &app->screen.gameplay.pause;
}

static overlay_menu_t *app_result_menu(app_t *app)
{
  return &app->screen.gameplay.result;
}

static menu_t *app_multiplayer_menu(app_t *app)
{
  return &app->screen.multiplayer.menu;
}

static overlay_menu_t *app_role_menu(app_t *app)
{
  return &app->screen.multiplayer.role_menu;
}

static bool app_is_menu_screen(app_screen_tag_t tag)
{
  return tag == APP_SCREEN_MENU ||
         tag == APP_SCREEN_INSTRUCTIONS ||
         tag == APP_SCREEN_CREDITS;
}

static bool app_is_multiplayer_setup_screen(app_screen_tag_t tag)
{
  return tag == APP_SCREEN_MULTIPLAYER_WAIT ||
         tag == APP_SCREEN_MULTIPLAYER_ROLE;
}

static bool app_is_multiplayer_game(const app_t *app)
{
  return app != NULL &&
         (app->tag == APP_SCREEN_PLAYING ||
          app->tag == APP_SCREEN_PAUSED ||
          app->tag == APP_SCREEN_GAME_OVER ||
          app->tag == APP_SCREEN_VICTORY) &&
         app->screen.gameplay.scene.multiplayer_role != MULTIPLAYER_ROLE_NONE;
}

static void app_set_screen(app_t *app, app_screen_tag_t tag)
{
  app->tag = tag;
  mouse_packet_reset();
}

static void app_shutdown_screen(app_t *app)
{
  switch (app->tag)
  {
    case APP_SCREEN_MENU:
    case APP_SCREEN_INSTRUCTIONS:
    case APP_SCREEN_CREDITS:
      menu_shutdown(app_menu(app));
      break;

    case APP_SCREEN_MULTIPLAYER_WAIT:
    case APP_SCREEN_MULTIPLAYER_ROLE:
      menu_shutdown(app_multiplayer_menu(app));
      break;

    case APP_SCREEN_PLAYING:
    case APP_SCREEN_PAUSED:
    case APP_SCREEN_GAME_OVER:
    case APP_SCREEN_VICTORY:
      scene_shutdown(app_scene(app));
      break;

    default:
      break;
  }

  app->tag = APP_SCREEN_EXIT;
}

static int app_switch_to_game(app_t *app, uint32_t current_tick, uint16_t level_id)
{
  app_shutdown_screen(app);

  if (scene_init(app_scene(app), &app->resources, level_id, APP_PHYSICS_HZ, APP_RENDER_HZ) != 0)
  {
    printf("app: scene_init() failed on transition to game\n");
    return 1;
  }

  app_set_screen(app, APP_SCREEN_PLAYING);
  app->last_physics_tick     = current_tick;
  app->last_render_tick      = current_tick;
  app->fps_window_start_tick = current_tick;
  app->frames_in_window      = 0;
  return 0;
}

static int app_switch_to_menu(app_t *app, uint32_t current_tick)
{
  app_shutdown_screen(app);

  if (menu_init(app_menu(app), &app->resources, APP_TIMER_HZ, APP_RENDER_HZ) != 0)
  {
    printf("app: menu_init() failed on transition to menu\n");
    return 1;
  }

  app_set_screen(app, APP_SCREEN_MENU);
  app->last_render_tick = current_tick;
  return 0;
}

static int app_switch_to_multiplayer_wait(app_t *app, uint32_t current_tick)
{
  app_shutdown_screen(app);

  if (menu_init(app_multiplayer_menu(app), &app->resources, APP_TIMER_HZ, APP_RENDER_HZ) != 0)
  {
    printf("app: menu_init() failed on transition to multiplayer wait\n");
    return 1;
  }

  if (app->serial_ready)
    ser_clear_rx_state(COM1_BASE);

  multiplayer_begin_wait(&app->multiplayer);
  app->multiplayer_level_id = LEVEL_FIRST_ID;
  app_set_screen(app, APP_SCREEN_MULTIPLAYER_WAIT);
  app->last_render_tick = current_tick;
  return 0;
}

static void app_switch_to_multiplayer_role(app_t *app, uint32_t current_tick)
{
  const uint16_t total_width = (uint16_t)(ROLE_BUTTON_WIDTH * 2U + ROLE_BUTTON_GAP);
  const uint16_t button_x = (uint16_t)((get_h_res() - total_width) / 2U);

  overlay_menu_init(app_role_menu(app), 2, button_x, 306,
                    ROLE_BUTTON_WIDTH, ROLE_BUTTON_HEIGHT, ROLE_BUTTON_GAP);
  app_set_screen(app, APP_SCREEN_MULTIPLAYER_ROLE);
  app->last_render_tick = current_tick;
}

static int app_switch_to_multiplayer_game(app_t *app, uint32_t current_tick, uint16_t level_id)
{
  multiplayer_role_t role = app->multiplayer.local_role;

  app_shutdown_screen(app);

  if (scene_init(app_scene(app), &app->resources, level_id, APP_PHYSICS_HZ, APP_RENDER_HZ) != 0)
  {
    printf("app: scene_init() failed on transition to multiplayer game\n");
    return 1;
  }

  scene_set_multiplayer_role(app_scene(app), role);
  multiplayer_mark_game_started(&app->multiplayer);
  app->multiplayer_level_id = level_id;
  if (app->pair_auto_role != MULTIPLAYER_ROLE_NONE)
    printf("ninjix pair started as %s\n", multiplayer_role_name(role));
  app_set_screen(app, APP_SCREEN_PLAYING);
  app->last_physics_tick     = current_tick;
  app->last_render_tick      = current_tick;
  app->fps_window_start_tick = current_tick;
  app->frames_in_window      = 0;
  if (app->pair_auto_role != MULTIPLAYER_ROLE_NONE)
    app->pair_auto_exit_tick = current_tick + APP_TIMER_HZ * 2U;
  return 0;
}

static void app_switch_to_exit(app_t *app)
{
  app_shutdown_screen(app);
}

static void app_leave_multiplayer_to_menu(app_t *app, uint32_t current_tick, bool notify_peer)
{
  if (notify_peer)
    multiplayer_send_leave(&app->multiplayer);
  else
    multiplayer_init(&app->multiplayer);

  if (app_switch_to_menu(app, current_tick) != 0)
    app_switch_to_exit(app);
}

static int app_handle_menu_result(app_t *app, menu_result_t result, uint32_t current_tick)
{
  switch (result)
  {
    case MENU_RESULT_PLAY_SINGLE:
      PROFILE_BEGIN("menu_result_start_game");
      if (app_switch_to_game(app, current_tick, LEVEL_FIRST_ID) != 0)
      {
        PROFILE_END();
        app_switch_to_exit(app);
        return 1;
      }
      PROFILE_END();
      return 0;

    case MENU_RESULT_PLAY_MULTI:
      PROFILE_BEGIN("menu_result_multiplayer_wait");
      app->multiplayer_level_id = LEVEL_FIRST_ID;
      if (app_switch_to_multiplayer_wait(app, current_tick) != 0)
      {
        PROFILE_END();
        app_switch_to_exit(app);
        return 1;
      }
      PROFILE_END();
      return 0;

    case MENU_RESULT_INSTRUCTIONS:
      PROFILE_BEGIN("menu_result_instructions");
      app_set_screen(app, APP_SCREEN_INSTRUCTIONS);
      PROFILE_END();
      return 0;

    case MENU_RESULT_CREDITS:
      PROFILE_BEGIN("menu_result_credits");
      app_set_screen(app, APP_SCREEN_CREDITS);
      PROFILE_END();
      return 0;

    case MENU_RESULT_BACK:
      PROFILE_BEGIN("menu_result_back");
      app_set_screen(app, APP_SCREEN_MENU);
      PROFILE_END();
      return 0;

    case MENU_RESULT_EXIT:
      PROFILE_BEGIN("menu_result_exit");
      app_switch_to_exit(app);
      PROFILE_END();
      return 0;

    default:
      PROFILE_BEGIN("menu_result_none");
      PROFILE_END();
      return 0;
  }
}

static void app_toggle_pause(app_t *app, uint32_t current_tick)
{
  if (app->tag == APP_SCREEN_PLAYING)
  {
    pause_menu_init(app_pause(app));
    if (app_is_multiplayer_game(app))
      multiplayer_request_pause(&app->multiplayer);
    app_set_screen(app, APP_SCREEN_PAUSED);
    app->last_render_tick = current_tick;
    return;
  }

  if (app->tag == APP_SCREEN_PAUSED)
  {
    if (app_is_multiplayer_game(app))
    {
      multiplayer_mark_local_resume_ready(&app->multiplayer);
      if (!multiplayer_both_resume_ready(&app->multiplayer))
      {
        app->last_render_tick = current_tick;
        return;
      }
    }

    app_set_screen(app, APP_SCREEN_PLAYING);
    app->last_physics_tick = current_tick;
    app->last_render_tick = current_tick;
    app->fps_window_start_tick = current_tick;
    app->frames_in_window = 0;
  }
}

static void app_handle_pause_action(app_t *app, pause_menu_action_t action, uint32_t current_tick)
{
  switch (action)
  {
    case PAUSE_MENU_ACTION_RESUME:
      app_toggle_pause(app, current_tick);
      break;

    case PAUSE_MENU_ACTION_CLOSE:
      if (app_is_multiplayer_game(app))
      {
        app_leave_multiplayer_to_menu(app, current_tick, true);
        break;
      }
      if (app_switch_to_menu(app, current_tick) != 0)
        app_switch_to_exit(app);
      break;

    default:
      break;
  }
}

static uint16_t app_result_option_count(const app_t *app)
{
  if (app != NULL && app->tag == APP_SCREEN_VICTORY &&
      levels_has_next(app->screen.gameplay.scene.play.level_id))
    return 3;

  return 2;
}

static uint16_t app_result_button_x(uint16_t option_count)
{
  uint16_t total_width = (uint16_t)(option_count * RESULT_BUTTON_WIDTH +
                                    (option_count - 1) * RESULT_BUTTON_GAP);
  return (uint16_t)((get_h_res() - total_width) / 2U);
}

static const char *const *app_result_labels(const app_t *app)
{
  static const char *const restart_close[] = {"RESTART", "MENU"};
  static const char *const restart_next_close[] = {"RESTART", "NEXT LVL", "MENU"};

  return app_result_option_count(app) == 3 ? restart_next_close : restart_close;
}

static void app_init_result_menu(app_t *app, uint16_t option_count)
{
  overlay_menu_init(app_result_menu(app), (uint8_t)option_count,
                    app_result_button_x(option_count), RESULT_BUTTON_Y,
                    RESULT_BUTTON_WIDTH, RESULT_BUTTON_HEIGHT,
                    RESULT_BUTTON_GAP);
}

static int app_handle_result_option(app_t *app, int16_t option, uint32_t current_tick)
{
  bool can_go_next = app != NULL && app->tag == APP_SCREEN_VICTORY &&
                     levels_has_next(app_scene(app)->play.level_id);
  uint16_t current_level_id = app_scene(app)->play.level_id;

  if (app_is_multiplayer_game(app) && option >= 0)
  {
    if (option == RESULT_OPTION_CLOSE || (!can_go_next && option == RESULT_OPTION_NEXT))
    {
      app_leave_multiplayer_to_menu(app, current_tick, true);
      return 0;
    }

    if (option == RESULT_OPTION_RESTART || option == RESULT_OPTION_NEXT)
    {
      multiplayer_result_action_t action =
          (option == RESULT_OPTION_RESTART) ? MULTIPLAYER_RESULT_ACTION_RESTART
                                            : MULTIPLAYER_RESULT_ACTION_NEXT;

      PROFILE_BEGIN("result_option_multiplayer_action");
      multiplayer_request_result_action(&app->multiplayer, action);

      if (app->multiplayer.local_result_action == app->multiplayer.peer_result_action &&
          app->multiplayer.local_result_action != MULTIPLAYER_RESULT_ACTION_NONE)
      {
        uint16_t target_level_id = current_level_id;
        if (action == MULTIPLAYER_RESULT_ACTION_NEXT)
          target_level_id = (uint16_t)(target_level_id + 1);

        if (app_switch_to_multiplayer_game(app, current_tick, target_level_id) != 0)
        {
          PROFILE_END();
          return 1;
        }
      }
      PROFILE_END();
      return 0;
    }
  }

  if (!can_go_next && option == 1)
    option = RESULT_OPTION_CLOSE;

  switch (option)
  {
    case RESULT_OPTION_RESTART:
      PROFILE_BEGIN("result_option_restart");
      if (app_switch_to_game(app, current_tick, current_level_id) != 0)
      {
        PROFILE_END();
        return 1;
      }
      PROFILE_END();
      return 0;

    case RESULT_OPTION_NEXT:
      if (can_go_next)
      {
        PROFILE_BEGIN("result_option_next_level");
        if (app_switch_to_game(app, current_tick, (uint16_t)(current_level_id + 1)) != 0)
        {
          PROFILE_END();
          return 1;
        }
        PROFILE_END();
        return 0;
      }

      PROFILE_BEGIN("result_option_next_menu");
      if (app_switch_to_menu(app, current_tick) != 0)
      {
        PROFILE_END();
        return 1;
      }
      PROFILE_END();
      return 0;

    case RESULT_OPTION_CLOSE:
      PROFILE_BEGIN("result_option_close_menu");
      if (app_switch_to_menu(app, current_tick) != 0)
      {
        PROFILE_END();
        return 1;
      }
      PROFILE_END();
      return 0;

    default:
      return 0;
  }
}

static void app_handle_play_result(app_t *app)
{
  switch (play_state_get_result(&app_scene(app)->play))
  {
    case PLAY_RESULT_GAME_OVER:
      app_set_screen(app, APP_SCREEN_GAME_OVER);
      app_init_result_menu(app, app_result_option_count(app));
      break;

    case PLAY_RESULT_VICTORY:
      app_set_screen(app, APP_SCREEN_VICTORY);
      app_init_result_menu(app, app_result_option_count(app));
      break;

    default:
      break;
  }
}

static void app_resume_multiplayer_game(app_t *app, uint32_t current_tick)
{
  app_set_screen(app, APP_SCREEN_PLAYING);
  app->last_physics_tick = current_tick;
  app->last_render_tick = current_tick;
  app->fps_window_start_tick = current_tick;
  app->frames_in_window = 0;
}

static const char *app_multiplayer_result_status(const app_t *app)
{
  if (app == NULL)
    return NULL;

  if (app->multiplayer.local_result_action == MULTIPLAYER_RESULT_ACTION_NONE)
    return "BOTH MUST APPROVE";

  if (app->multiplayer.peer_result_action == MULTIPLAYER_RESULT_ACTION_NONE)
    return "WAITING FOR PEER";

  if (app->multiplayer.local_result_action != app->multiplayer.peer_result_action)
    return "CHOICES MUST MATCH";

  return "RESTARTING...";
}

static int app_try_complete_multiplayer_result_action(app_t *app, uint32_t current_tick)
{
  if (app == NULL || !app_is_multiplayer_game(app))
    return 0;

  if (app->tag != APP_SCREEN_GAME_OVER && app->tag != APP_SCREEN_VICTORY)
    return 0;

  if (app->multiplayer.local_result_action == MULTIPLAYER_RESULT_ACTION_NONE ||
      app->multiplayer.local_result_action != app->multiplayer.peer_result_action)
    return 0;

  uint16_t next_level_id = app->multiplayer_level_id;

  switch (app->multiplayer.local_result_action)
  {
    case MULTIPLAYER_RESULT_ACTION_RESTART:
      break;

    case MULTIPLAYER_RESULT_ACTION_NEXT:
      if (app->tag != APP_SCREEN_VICTORY || !levels_has_next(next_level_id))
        return 0;
      next_level_id = (uint16_t)(next_level_id + 1);
      break;

    default:
      return 0;
  }

  if (app_switch_to_multiplayer_game(app, current_tick, next_level_id) != 0)
    return 1;

  return 0;
}

static void app_handle_multiplayer_event(app_t *app, const multiplayer_event_t *event)
{
  if (app == NULL || event == NULL)
    return;

  uint32_t current_tick = timer_get_counter();

  switch (event->type)
  {
    case MULTIPLAYER_EVENT_CONNECTED:
      if (app->tag == APP_SCREEN_MULTIPLAYER_WAIT)
        app_switch_to_multiplayer_role(app, current_tick);
      break;

    case MULTIPLAYER_EVENT_ROLE_CHANGED:
    case MULTIPLAYER_EVENT_START_GAME:
      if (app->tag == APP_SCREEN_MULTIPLAYER_ROLE &&
          multiplayer_roles_ready(&app->multiplayer) &&
          app_switch_to_multiplayer_game(app, current_tick, app->multiplayer_level_id) != 0)
        app_switch_to_exit(app);
      break;

    case MULTIPLAYER_EVENT_TOWER_PLACED:
      if (app_is_multiplayer_game(app) &&
          app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_ATTACKER)
        (void)scene_apply_remote_tower(app_scene(app),
                                       event->data.tower.kind,
                                       event->data.tower.x,
                                       event->data.tower.y);
      break;

    case MULTIPLAYER_EVENT_TOWER_UPGRADED:
      if (app_is_multiplayer_game(app) &&
          app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_ATTACKER)
        (void)scene_apply_remote_tower_upgrade(app_scene(app),
                                               event->data.tower_upgrade.index,
                                               event->data.tower_upgrade.level);
      break;

    case MULTIPLAYER_EVENT_TOWER_SOLD:
      if (app_is_multiplayer_game(app) &&
          app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_ATTACKER)
        (void)scene_apply_remote_tower_sell(app_scene(app),
                                            event->data.tower_sell.index);
      break;

    case MULTIPLAYER_EVENT_TOWER_TARGET_CHANGED:
      if (app_is_multiplayer_game(app) &&
          app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_ATTACKER)
        (void)scene_apply_remote_tower_target(app_scene(app),
                                              event->data.tower_target.index,
                                              event->data.tower_target.mode);
      break;

    case MULTIPLAYER_EVENT_ENEMY_SPAWNED:
      if (app_is_multiplayer_game(app) &&
          app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_DEFENDER)
        (void)scene_apply_remote_enemy(app_scene(app), event->data.enemy.kind, event->data.enemy.wave);
      break;

    case MULTIPLAYER_EVENT_PAUSE_REQUESTED:
      if (app->tag == APP_SCREEN_PLAYING && app_is_multiplayer_game(app))
      {
        pause_menu_init(app_pause(app));
        app_set_screen(app, APP_SCREEN_PAUSED);
        app->last_render_tick = current_tick;
      }
      break;

    case MULTIPLAYER_EVENT_RESUME_READY:
      if (app->tag == APP_SCREEN_PAUSED &&
          app_is_multiplayer_game(app) &&
          multiplayer_both_resume_ready(&app->multiplayer))
        app_resume_multiplayer_game(app, current_tick);
      break;

    case MULTIPLAYER_EVENT_RESULT_ACTION:
      if (app_try_complete_multiplayer_result_action(app, current_tick) != 0)
        app_switch_to_exit(app);
      break;

    case MULTIPLAYER_EVENT_LEAVE:
      if (app->tag == APP_SCREEN_MULTIPLAYER_WAIT)
      {
        multiplayer_begin_wait(&app->multiplayer);
        app->last_render_tick = current_tick;
      }
      else if (app_is_multiplayer_setup_screen(app->tag) || app_is_multiplayer_game(app))
        app_leave_multiplayer_to_menu(app, current_tick, false);
      break;

    default:
      break;
  }
}

static void app_drain_multiplayer_events(app_t *app)
{
  multiplayer_event_t event;

  while (multiplayer_next_event(&app->multiplayer, &event))
    app_handle_multiplayer_event(app, &event);
}

static void app_service_multiplayer_serial(app_t *app)
{
  if (app == NULL || !app->serial_ready)
    return;

  ser_ih();
  multiplayer_poll_serial(&app->multiplayer);
  app_drain_multiplayer_events(app);
}

static void app_cleanup(app_t *app)
{
  if (app == NULL)
    return;

  if (app->mouse_reporting)
    mouse_disable_data_report();
  if (app->mouse_ready)
    mouse_unsubscribe_int();
  if (app->serial_ready)
    ser_unsubscribe_int();
  if (app->keyboard_ready)
    kbc_unsubscribe_int();
  if (app->timer_ready)
    timer_unsubscribe_int();

  app_shutdown_screen(app);
  resources_destroy(&app->resources);

  if (app->graphics_ready)
  {
    vg_dispose_buffer();
    vg_exit();
    util_force_tty_redraw();
  }
}



static void app_update_fps(app_t *app, uint32_t current_tick)
{
  uint32_t elapsed = current_tick - app->fps_window_start_tick;
  if (elapsed < APP_TIMER_HZ)
    return;

  if (elapsed > 0)
    app_scene(app)->display_fps =
        (uint16_t)((app->frames_in_window * APP_TIMER_HZ + elapsed / 2) / elapsed);

  app->frames_in_window      = 0;
  app->fps_window_start_tick = current_tick;
}



static int app_handle_timer_event(app_t *app, uint32_t current_tick)
{
  if (app_is_menu_screen(app->tag))
  {
    PROFILE_BEGIN("menu_update");
    menu_result_t result = menu_update(app_menu(app), current_tick);
    PROFILE_END();

    PROFILE_BEGIN("menu_transition");
    if (app_handle_menu_result(app, result, current_tick) != 0)
    {
      PROFILE_END();
      return 1;
    }
    PROFILE_END();

    if (!app_is_menu_screen(app->tag))
      return 0;

    if ((current_tick - app->last_render_tick) < app->render_ticks)
      // we're still within the same render tick, no need to redraw yet
      return 0;

    PROFILE_BEGIN("menu_render");
    if (menu_render(app_menu(app)) != 0)
    {
      PROFILE_END();
      printf("app: menu_render() failed\n");
      return 1;
    }
    PROFILE_END();

    app_advance_render_ticks(app, current_tick);

    return 0;
  }

  if (app_is_multiplayer_setup_screen(app->tag))
  {
    app_service_multiplayer_serial(app);
    if (!app_is_multiplayer_setup_screen(app->tag))
      return 0;

    PROFILE_BEGIN("multiplayer_tick");
    multiplayer_tick(&app->multiplayer, current_tick);
    PROFILE_END();

    if (app->tag == APP_SCREEN_MULTIPLAYER_WAIT && app->multiplayer.peer_connected)
      app_switch_to_multiplayer_role(app, current_tick);

    if (app->tag == APP_SCREEN_MULTIPLAYER_ROLE &&
        app->pair_auto_role != MULTIPLAYER_ROLE_NONE &&
        app->multiplayer.local_role == MULTIPLAYER_ROLE_NONE)
      multiplayer_select_role(&app->multiplayer, app->pair_auto_role);

    if (app->tag == APP_SCREEN_MULTIPLAYER_ROLE && multiplayer_roles_ready(&app->multiplayer))
    {
      if (app_switch_to_multiplayer_game(app, current_tick, app->multiplayer_level_id) != 0)
        return 1;
      return 0;
    }

    if ((current_tick - app->last_render_tick) < app->render_ticks)
      return 0;

    if (app->tag == APP_SCREEN_MULTIPLAYER_WAIT)
    {
      const char *status = app->serial_ready ?
                           "WAITING FOR CONNECTION..." :
                           "SERIAL PORT UNAVAILABLE";
      if (menu_render_multiplayer_wait(app_multiplayer_menu(app), status) != 0)
        return 1;
    }
    else if (app->tag == APP_SCREEN_MULTIPLAYER_ROLE)
    {
      const char *status = "CHOOSE YOUR ROLE";
      if (app->multiplayer.local_role != MULTIPLAYER_ROLE_NONE &&
          app->multiplayer.peer_role == MULTIPLAYER_ROLE_NONE)
        status = "WAITING FOR OTHER PLAYER...";
      else if (app->multiplayer.local_role != MULTIPLAYER_ROLE_NONE &&
               app->multiplayer.local_role == app->multiplayer.peer_role)
        status = "BOTH CHOSE THE SAME ROLE";
      else if (multiplayer_roles_ready(&app->multiplayer))
        status = "STARTING...";

      if (menu_render_multiplayer_roles(app_multiplayer_menu(app), app_role_menu(app),
                                        app->multiplayer.local_role,
                                        app->multiplayer.peer_role,
                                        status) != 0)
        return 1;
    }

    app_advance_render_ticks(app, current_tick);

    return 0;
  }

  if (app_is_multiplayer_game(app))
  {
    app_service_multiplayer_serial(app);
    if (!app_is_multiplayer_game(app))
      return 0;
  }

  if (app->tag == APP_SCREEN_PAUSED)
  {
    PROFILE_BEGIN("paused_cursor");
    scene_step_cursor(app_scene(app));
    PROFILE_END();

    PROFILE_BEGIN("paused_hover");
    pause_menu_handle_mouse_move(app_pause(app), app_scene(app)->cursor_x, app_scene(app)->cursor_y);
    PROFILE_END();

    if ((current_tick - app->last_render_tick) < app->render_ticks)
      return 0;

    PROFILE_BEGIN("paused_render");
    const char *pause_status = NULL;
    if (app_is_multiplayer_game(app))
      pause_status = app->multiplayer.local_resume_ready ?
                     "WAITING FOR PEER" : "BOTH MUST RESUME";

    int render_result = app_is_multiplayer_game(app) ?
        scene_render_paused_multiplayer(app_scene(app), app_pause(app), pause_status) :
        scene_render_paused(app_scene(app), app_pause(app));

    if (render_result != 0)
    {
      PROFILE_END();
      printf("app: scene_render_paused() failed\n");
      return 1;
    }
    PROFILE_END();

    app_advance_render_ticks(app, current_tick);

    return 0;
  }

  if (app->tag == APP_SCREEN_GAME_OVER || app->tag == APP_SCREEN_VICTORY)
  {
    PROFILE_BEGIN("result_cursor");
    scene_step_cursor(app_scene(app));
    PROFILE_END();

    if ((current_tick - app->last_render_tick) < app->render_ticks)
      return 0;

    const char *title = (app->tag == APP_SCREEN_VICTORY) ? "VICTORY" : "GAME OVER";
    if (app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_ATTACKER)
      title = (app->tag == APP_SCREEN_VICTORY) ? "DEFEAT" : "VICTORY";
    else if (app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_DEFENDER)
      title = (app->tag == APP_SCREEN_VICTORY) ? "VICTORY" : "DEFEAT";
      
    const char *status_text = app_is_multiplayer_game(app) ?
                              app_multiplayer_result_status(app) : NULL;
    PROFILE_BEGIN("result_render");
    if (scene_render_result(app_scene(app), title,
                            app_result_menu(app), app_result_labels(app), status_text) != 0)
    {
      PROFILE_END();
      printf("app: scene_render_result() failed\n");
      return 1;
    }
    PROFILE_END();

    app_advance_render_ticks(app, current_tick);

    return 0;
  }

  if (app->tag != APP_SCREEN_PLAYING)
    return 0;

  app_update_fps(app, current_tick);

  if ((current_tick - app->last_physics_tick) >= app->physics_ticks)
  {
    PROFILE_BEGIN("physics");
    scene_step_physics(app_scene(app));
    PROFILE_END();
    app->last_physics_tick = current_tick;
    app_handle_play_result(app);
    if (app->tag != APP_SCREEN_PLAYING)
      return 0;
  }

  if ((current_tick - app->last_render_tick) < app->render_ticks)
    return 0;

  PROFILE_BEGIN("render");
  if (scene_render(app_scene(app)) != 0)
  {
    PROFILE_END();
    printf("app: scene_render() failed\n");
    return 1;
  }
  PROFILE_END();

  app->frames_in_window++;
  app_advance_render_ticks(app, current_tick);

  if (app->pair_auto_exit_tick != 0 && current_tick >= app->pair_auto_exit_tick)
  {
    printf("ninjix pair smoke complete\n");
    app_switch_to_exit(app);
  }

  return 0;
}

static void app_handle_key_event(app_t *app, const app_event_t *event)
{
  const bool make = event->data.key.make;
  const uint8_t scancode = event->data.key.scancode;

  if (app_is_menu_screen(app->tag))
  {
    PROFILE_BEGIN("menu_key");
    menu_result_t result = menu_handle_keyboard(app_menu(app), make, scancode);
    PROFILE_END();

    PROFILE_BEGIN("menu_key_transition");
    app_handle_menu_result(app, result, timer_get_counter());
    PROFILE_END();
    return;
  }

  if (app_is_multiplayer_setup_screen(app->tag))
  {
    if (!make && scancode == ESC_BREAK)
    {
      app_leave_multiplayer_to_menu(app, timer_get_counter(), app->multiplayer.peer_connected);
      return;
    }

    if (app->tag == APP_SCREEN_MULTIPLAYER_ROLE)
    {
      int16_t option = overlay_menu_handle_keyboard(app_role_menu(app), make, scancode);
      if (option == 0)
        multiplayer_select_role(&app->multiplayer, MULTIPLAYER_ROLE_ATTACKER);
      else if (option == 1)
        multiplayer_select_role(&app->multiplayer, MULTIPLAYER_ROLE_DEFENDER);

      if (multiplayer_roles_ready(&app->multiplayer) &&
          app_switch_to_multiplayer_game(app, timer_get_counter(), app->multiplayer_level_id) != 0)
        app_switch_to_exit(app);
    }
    return;
  }

  if (app->tag == APP_SCREEN_PLAYING)
  {
    PROFILE_BEGIN("game_key");
    if (make && scancode == SCANCODE_N && app_scene(app)->multiplayer_role == MULTIPLAYER_ROLE_NONE)
    {
      uint16_t current = app_scene(app)->play.level_id;
      uint16_t next = (current >= LEVEL_COUNT) ? LEVEL_FIRST_ID : (uint16_t)(current + 1);
      PROFILE_END();
      if (app_switch_to_game(app, timer_get_counter(), next) != 0)
        app_switch_to_exit(app);
      return;
    }
    if (!make && scancode == ESC_BREAK)
    {
      PROFILE_END();
      PROFILE_BEGIN("game_key_transition");
      app_toggle_pause(app, timer_get_counter());
      PROFILE_END();
      return;
    }
    PROFILE_END();

    if (make && scancode >= SCANCODE_1 && scancode <= SCANCODE_4 &&
        app_scene(app)->multiplayer_role != MULTIPLAYER_ROLE_ATTACKER)
    {
      tower_kind_t kind = (tower_kind_t)(scancode - SCANCODE_1);
      play_state_t *play = &app_scene(app)->play;
      if (play->placement_active && play->placement_kind == kind)
        play_state_cancel_placement(play);
      else
        play_state_begin_placement(play, kind);
    }

    return;
  }

  if (app->tag == APP_SCREEN_PAUSED)
  {
    PROFILE_BEGIN("paused_key");
    if (!make && scancode == ESC_BREAK)
    {
      PROFILE_END();
      PROFILE_BEGIN("paused_key_transition");
      app_toggle_pause(app, timer_get_counter());
      PROFILE_END();
      return;
    }

    pause_menu_action_t action = pause_menu_handle_keyboard(app_pause(app), make, scancode);
    PROFILE_END();

    PROFILE_BEGIN("paused_key_transition");
    app_handle_pause_action(app, action, timer_get_counter());
    PROFILE_END();
    return;
  }

  if (app->tag == APP_SCREEN_GAME_OVER || app->tag == APP_SCREEN_VICTORY)
  {
    PROFILE_BEGIN("result_key");
    if (!make && scancode == ESC_BREAK)
    {
      PROFILE_END();
      PROFILE_BEGIN("result_key_transition");
      PROFILE_BEGIN("result_key_transition_menu");
      if (app_is_multiplayer_game(app))
        app_leave_multiplayer_to_menu(app, timer_get_counter(), true);
      else if (app_switch_to_menu(app, timer_get_counter()) != 0)
        app_switch_to_exit(app);
      PROFILE_END();
      PROFILE_END();
      return;
    }
    PROFILE_END();

    PROFILE_BEGIN("result_key_option");
    if (app_handle_result_option(app,
                                 overlay_menu_handle_keyboard(app_result_menu(app),
                                                              make, scancode),
                                 timer_get_counter()) != 0)
      app_switch_to_exit(app);
    PROFILE_END();
  }
}

static void app_handle_mouse_event(app_t *app, const app_event_t *event)
{
  const int16_t dx = event->data.mouse.dx;
  const int16_t dy = event->data.mouse.dy;

  if (app_is_menu_screen(app->tag))
  {
    PROFILE_BEGIN("menu_mouse_move");
    menu_handle_mouse_movement(app_menu(app), dx, dy);
    PROFILE_END();

    if (event->data.mouse.left_button)
    {
      PROFILE_BEGIN("menu_mouse_click");
      menu_result_t result = menu_handle_mouse_click(app_menu(app));
      PROFILE_END();

      PROFILE_BEGIN("menu_mouse_transition");
      app_handle_menu_result(app, result, timer_get_counter());
      PROFILE_END();
    }
    return;
  }

  if (app_is_multiplayer_setup_screen(app->tag))
  {
    menu_handle_mouse_movement(app_multiplayer_menu(app), dx, dy);

    if (app->tag == APP_SCREEN_MULTIPLAYER_ROLE)
    {
      overlay_menu_handle_mouse_move(app_role_menu(app),
                                     app_multiplayer_menu(app)->cursor_x,
                                     app_multiplayer_menu(app)->cursor_y);

      if (event->data.mouse.left_button)
      {
        int16_t option = overlay_menu_handle_mouse_click(app_role_menu(app));
        if (option == 0)
          multiplayer_select_role(&app->multiplayer, MULTIPLAYER_ROLE_ATTACKER);
        else if (option == 1)
          multiplayer_select_role(&app->multiplayer, MULTIPLAYER_ROLE_DEFENDER);

        if (multiplayer_roles_ready(&app->multiplayer) &&
            app_switch_to_multiplayer_game(app, timer_get_counter(), app->multiplayer_level_id) != 0)
          app_switch_to_exit(app);
      }
    }
    return;
  }

  if (app->tag == APP_SCREEN_PLAYING)
  {
    scene_t *scene;

    PROFILE_BEGIN("game_mouse_accumulate");
    scene = app_scene(app);
    scene_accumulate_mouse_motion(app_scene(app), dx, dy,
                                  event->data.mouse.x_overflow,
                                  event->data.mouse.y_overflow);
    PROFILE_END();

    PROFILE_BEGIN("game_mouse_cursor");
    scene_step_cursor(scene);
    PROFILE_END();

    if (event->data.mouse.right_button)
      scene_handle_placement_cancel(scene);
    if (event->data.mouse.left_button && !scene->prev_left_button)
    {
      if (app_is_multiplayer_game(app))
      {
        scene_multiplayer_action_t action;
        scene_handle_multiplayer_mouse_click(scene, &action);
        if (action.type == MULTIPLAYER_EVENT_TOWER_PLACED)
          multiplayer_send_tower(&app->multiplayer,
                                 action.data.tower.kind,
                                 action.data.tower.x,
                                 action.data.tower.y);
        else if (action.type == MULTIPLAYER_EVENT_TOWER_UPGRADED)
          multiplayer_send_tower_upgrade(&app->multiplayer,
                                         action.data.tower_upgrade.index,
                                         action.data.tower_upgrade.level);
        else if (action.type == MULTIPLAYER_EVENT_TOWER_SOLD)
          multiplayer_send_tower_sell(&app->multiplayer,
                                      action.data.tower_sell.index);
        else if (action.type == MULTIPLAYER_EVENT_TOWER_TARGET_CHANGED)
          multiplayer_send_tower_target(&app->multiplayer,
                                        action.data.tower_target.index,
                                        action.data.tower_target.mode);
        else if (action.type == MULTIPLAYER_EVENT_ENEMY_SPAWNED)
          multiplayer_send_enemy(&app->multiplayer, action.data.enemy.kind, action.data.enemy.wave);
      }
      else
      {
        scene_handle_mouse_click(scene);
      }
    }
    scene->prev_left_button = event->data.mouse.left_button;
  }

  if (app->tag == APP_SCREEN_PAUSED)
  {
    PROFILE_BEGIN("paused_mouse_accumulate");
    scene_accumulate_mouse_motion(app_scene(app), dx, dy,
                                  event->data.mouse.x_overflow,
                                  event->data.mouse.y_overflow);
    PROFILE_END();

    PROFILE_BEGIN("paused_mouse_cursor");
    scene_step_cursor(app_scene(app));
    PROFILE_END();

    PROFILE_BEGIN("paused_mouse_hover");
    pause_menu_handle_mouse_move(app_pause(app), app_scene(app)->cursor_x, app_scene(app)->cursor_y);
    PROFILE_END();

    if (event->data.mouse.left_button)
    {
      PROFILE_BEGIN("paused_mouse_click");
      pause_menu_action_t action = pause_menu_handle_mouse_click(app_pause(app));
      app_handle_pause_action(app, action, timer_get_counter());
      PROFILE_END();
    }
    return;
  }

  if (app->tag == APP_SCREEN_GAME_OVER || app->tag == APP_SCREEN_VICTORY)
  {
    PROFILE_BEGIN("result_mouse_accumulate");
    scene_accumulate_mouse_motion(app_scene(app), dx, dy,
                                  event->data.mouse.x_overflow,
                                  event->data.mouse.y_overflow);
    PROFILE_END();

    PROFILE_BEGIN("result_mouse_cursor");
    scene_step_cursor(app_scene(app));
    PROFILE_END();

    PROFILE_BEGIN("result_mouse_hover");
    overlay_menu_handle_mouse_move(app_result_menu(app),
                                   app_scene(app)->cursor_x,
                                   app_scene(app)->cursor_y);
    PROFILE_END();

    if (event->data.mouse.left_button)
    {
      PROFILE_BEGIN("result_mouse_click");
      if (app_handle_result_option(app,
                                   overlay_menu_handle_mouse_click(app_result_menu(app)),
                                   timer_get_counter()) != 0)
        app_switch_to_exit(app);
      PROFILE_END();
    }
  }
}

static int app_handle_event(app_t *app, const app_event_t *event)
{
  switch (event->kind)
  {
    case APP_EVENT_TIMER:
      return app_handle_timer_event(app, event->data.timer.tick);

    case APP_EVENT_KEY:
      app_handle_key_event(app, event);
      return 0;

    case APP_EVENT_MOUSE:
      app_handle_mouse_event(app, event);
      return 0;

    default:
      return 0;
  }
}



static void app_handle_keyboard_interrupt(app_t *app)
{
  PROFILE_BEGIN("kbd_read_hw");
  kbc_ih();
  if (kbc_had_error())
  {
    PROFILE_END();
    return;
  }

  uint8_t byte;
  if (kbc_get_byte(&byte) != 0)
  {
    PROFILE_END();
    return;
  }
  PROFILE_END();

  PROFILE_BEGIN("kbd_scancode_byte");
  if (!kbd_process_byte(byte))
  {
    PROFILE_END();
    return;
  }
  PROFILE_END();

  PROFILE_BEGIN("kbd_decode_scancode");
  bool    make;
  uint8_t size;
  uint8_t bytes[2];
  if (kbd_get_scancode(&make, &size, bytes) != 0)
  {
    PROFILE_END();
    return;
  }
  PROFILE_END();

  PROFILE_BEGIN("kbd_event");
  app_event_t event;
  memset(&event, 0, sizeof(event));
  event.kind = APP_EVENT_KEY;
  event.data.key.make = make;
  event.data.key.scancode = (size == 2) ? bytes[1] : bytes[0];
  app_handle_event(app, &event);
  PROFILE_END();
}



static void app_handle_mouse_interrupt(app_t *app)
{
  PROFILE_BEGIN("mouse_read_hw");
  mouse_ih();
  if (mouse_had_error())
  {
    mouse_packet_reset();
    PROFILE_END();
    return;
  }

  uint8_t byte;
  if (mouse_get_byte(&byte) != 0)
  {
    mouse_packet_reset();
    PROFILE_END();
    return;
  }
  PROFILE_END();

  PROFILE_BEGIN("mouse_packet_byte");
  if (!mouse_process_byte(byte))
  {
    PROFILE_END();
    return;
  }
  PROFILE_END();

  PROFILE_BEGIN("mouse_decode_packet");
  struct packet packet;
  if (mouse_get_packet(&packet) != 0)
  {
    PROFILE_END();
    return;
  }
  PROFILE_END();

  PROFILE_BEGIN("mouse_event");
  app_event_t event;
  memset(&event, 0, sizeof(event));
  event.kind = APP_EVENT_MOUSE;
  event.data.mouse.dx = (int16_t)packet.delta_x;
  event.data.mouse.dy = (int16_t)packet.delta_y;
  event.data.mouse.left_button = packet.lb;
  event.data.mouse.right_button = packet.rb;
  event.data.mouse.middle_button = packet.mb;
  event.data.mouse.x_overflow = packet.x_ov;
  event.data.mouse.y_overflow = packet.y_ov;
  app_handle_event(app, &event);
  PROFILE_END();
}



static int app_init(app_t *app)
{
  memset(app, 0, sizeof(*app));
  app->tag = APP_SCREEN_EXIT;
  multiplayer_init(&app->multiplayer);
  app->multiplayer_level_id = LEVEL_FIRST_ID;
  const char *pair_auto = getenv("MACHINE_LAB_PAIR_AUTO");
  const char *pair_side = getenv("MACHINE_LAB_PAIR_SIDE");
  if (pair_auto == NULL || pair_auto[0] == '\0')
    pair_auto = getenv("LCOM_PAIR_AUTO");
  if (pair_side == NULL || pair_side[0] == '\0')
    pair_side = getenv("LCOM_PAIR_SIDE");
  if (pair_auto != NULL && pair_auto[0] != '\0' && strcmp(pair_auto, "0") != 0)
  {
    if (pair_side != NULL && strcmp(pair_side, "left") == 0)
      app->pair_auto_role = MULTIPLAYER_ROLE_ATTACKER;
    else if (pair_side != NULL && strcmp(pair_side, "right") == 0)
      app->pair_auto_role = MULTIPLAYER_ROLE_DEFENDER;
  }

  if (vg_init(APP_VIDEO_MODE) == NULL)
  {
    printf("app_init(): vg_init() failed\n");
    return 1;
  }
  app->graphics_ready = true;

  if (vg_prepare_buffer() != 0)
  {
    printf("app_init(): vg_prepare_buffer() failed\n");
    app_cleanup(app);
    return 1;
  }

  if (resources_load(&app->resources) != 0)
  {
    printf("app_init(): resources_load() failed\n");
    app_cleanup(app);
    return 1;
  }

  if (menu_init(app_menu(app), &app->resources, APP_TIMER_HZ, APP_RENDER_HZ) != 0)
  {
    printf("app_init(): menu_init() failed\n");
    app_cleanup(app);
    return 1;
  }
  app->tag = APP_SCREEN_MENU;

  timer_set_counter(0);

  if (timer_subscribe_int(&app->timer_bit_no) != 0)
  {
    printf("app_init(): timer_subscribe_int() failed\n");
    app_cleanup(app);
    return 1;
  }
  app->timer_ready = true;

  if (kbc_subscribe_int(&app->keyboard_bit_no) != 0)
  {
    printf("app_init(): kbc_subscribe_int() failed\n");
    app_cleanup(app);
    return 1;
  }
  app->keyboard_ready = true;

  if (mouse_subscribe_int(&app->mouse_bit_no) != 0)
  {
    printf("app_init(): mouse_subscribe_int() failed\n");
    app_cleanup(app);
    return 1;
  }
  app->mouse_ready = true;

  if (mouse_enable_data_report() != 0)
  {
    printf("app_init(): mouse_enable_data_report() failed\n");
    app_cleanup(app);
    return 1;
  }
  app->mouse_reporting = true;
  mouse_packet_reset();

  if (ser_set_impl(COM1_BASE, 115200, 8, 1, 0) == 0)
  {
    (void)ser_enable_fifo(COM1_BASE, FCR_TRIG_14);
    if (ser_subscribe_int(COM1_BASE, &app->serial_bit_no) == 0)
      app->serial_ready = true;
    else
    {
      (void)ser_unsubscribe_int();
      printf("app_init(): serial subscribe failed (multiplayer disabled)\n");
    }
  }
  else
  {
    printf("app_init(): serial configuration failed (multiplayer disabled)\n");
  }

  if (timer_calc_ticks_per_event(APP_TIMER_HZ, APP_PHYSICS_HZ, &app->physics_ticks) != 0 ||
      timer_calc_ticks_per_event(APP_TIMER_HZ, APP_RENDER_HZ,  &app->render_ticks)  != 0)
  {
    printf("app_init(): timer_calc_ticks_per_event() failed\n");
    app_cleanup(app);
    return 1;
  }

  app->last_physics_tick     = timer_get_counter();
  app->last_render_tick      = app->last_physics_tick;
  app->fps_window_start_tick = app->last_physics_tick;
  app->frames_in_window      = 0;

  if (app->pair_auto_role != MULTIPLAYER_ROLE_NONE)
  {
    if (app_switch_to_multiplayer_wait(app, app->last_physics_tick) != 0)
    {
      app_cleanup(app);
      return 1;
    }
  }

  int initial_render_result = 0;
  if (app->tag == APP_SCREEN_MULTIPLAYER_WAIT)
    initial_render_result = menu_render_multiplayer_wait(app_multiplayer_menu(app),
                                                         "WAITING FOR CONNECTION...");
  else
    initial_render_result = menu_render(app_menu(app));

  if (initial_render_result != 0)
  {
    printf("app_init(): initial menu render failed\n");
    app_cleanup(app);
    return 1;
  }

  return 0;
}



int app_run(void)
{
  app_t app;
  if (app_init(&app) != 0)
    return 1;

  PROFILE_RESET();

  int      result        = 0;
  uint32_t timer_irq_set    = BIT(app.timer_bit_no);
  uint32_t keyboard_irq_set = BIT(app.keyboard_bit_no);
  uint32_t mouse_irq_set    = BIT(app.mouse_bit_no);
  uint32_t serial_irq_set   = app.serial_ready ? BIT(app.serial_bit_no) : 0;

  while (app.tag != APP_SCREEN_EXIT)
  {
    lcom_event_t event_mask;
    if (lcom_event_wait(&event_mask) != LCOM_OK)
    {
      printf("app_run(): lcom_event_wait() failed\n");
      continue;
    }

    if (event_mask.irq_mask & keyboard_irq_set)
    {
      PROFILE_BEGIN("keyboard_irq");
      app_handle_keyboard_interrupt(&app);
      PROFILE_END();
    }

    if (event_mask.irq_mask & mouse_irq_set)
    {
      PROFILE_BEGIN("mouse_irq");
      app_handle_mouse_interrupt(&app);
      PROFILE_END();
    }

    if (app.serial_ready && (event_mask.irq_mask & serial_irq_set))
    {
      PROFILE_BEGIN("serial_irq");
      ser_ih();
      if (app_is_multiplayer_setup_screen(app.tag) || app_is_multiplayer_game(&app))
      {
        multiplayer_poll_serial(&app.multiplayer);
        app_drain_multiplayer_events(&app);
      }
      else
      {
        uint8_t ignored_byte;
        uint16_t drain_limit = 256;
        while (ser_recv_char_int(&ignored_byte) == 0 && drain_limit-- > 0)
          ;
      }
      PROFILE_END();
    }

    if (event_mask.irq_mask & timer_irq_set)
    {
      PROFILE_BEGIN("timer_irq");
      timer_int_handler();

      app_event_t event;
      memset(&event, 0, sizeof(event));
      event.kind = APP_EVENT_TIMER;
      event.data.timer.tick = timer_get_counter();

      PROFILE_BEGIN("frame");
      if (app_handle_event(&app, &event) != 0)
      {
        PROFILE_END();
        PROFILE_END();
        result = 1;
        app_switch_to_exit(&app);
        continue;
      }
      PROFILE_END();
      PROFILE_END();
    }
  }

  app_cleanup(&app);
  PROFILE_REPORT();
  return result;
}
