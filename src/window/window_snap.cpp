#include "window/window_snap.hpp"

#include "core/constants.hpp"
#include "ui/ui_utils.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr int WINDOW_MARGIN = 32;
constexpr int MIN_WINDOW_W  = 640;
constexpr int MIN_WINDOW_H  = 480;

SDL_DisplayID display_for_window(SDL_Window* win) {
  SDL_DisplayID disp = SDL_GetDisplayForWindow(win);
  if (!disp) disp = SDL_GetPrimaryDisplay();
  return disp;
}

SDL_Rect usable_bounds(SDL_DisplayID disp) {
  SDL_Rect bounds { 0, 0, 1280, 720 };
  if (disp) {
    if (!SDL_GetDisplayUsableBounds(disp, &bounds))
      SDL_GetDisplayBounds(disp, &bounds);
  }
  return bounds;
}

void clamp_size_to_bounds(int& w, int& h, const SDL_Rect& usable, int margin) {
  int max_w = std::max(MIN_WINDOW_W, usable.w - margin);
  int max_h = std::max(MIN_WINDOW_H, usable.h - margin);
  w = std::clamp(w, MIN_WINDOW_W, max_w);
  h = std::clamp(h, MIN_WINDOW_H, max_h);
}

void clamp_position_to_bounds(int& x, int& y, int w, int h, const SDL_Rect& usable) {
  int max_x = usable.x + usable.w - w;
  int max_y = usable.y + usable.h - h;
  if (max_x < usable.x) max_x = usable.x;
  if (max_y < usable.y) max_y = usable.y;
  x = std::clamp(x, usable.x, max_x);
  y = std::clamp(y, usable.y, max_y);
}

SDL_Rect display_bounds_at_mouse() {
  float gx = 0.f, gy = 0.f;
  SDL_GetGlobalMouseState(&gx, &gy);
  SDL_Point pt{(int)gx, (int)gy};
  SDL_DisplayID disp = SDL_GetDisplayForPoint(&pt);
  if (!disp) disp = SDL_GetPrimaryDisplay();
  return usable_bounds(disp);
}

void read_window_rect(AppState* state, int& x, int& y, int& w, int& h) {
  SDL_GetWindowPosition(state->window, &x, &y);
  SDL_GetWindowSize(state->window, &w, &h);
}

bool rect_matches_display(int x, int y, int w, int h, const SDL_Rect& db) {
  constexpr int tol = 8;
  return std::abs(x - db.x) <= tol &&
         std::abs(y - db.y) <= tol &&
         std::abs(w - db.w) <= tol &&
         std::abs(h - db.h) <= tol;
}

void default_windowed_rect(AppState* state, int& x, int& y, int& w, int& h) {
  SDL_Rect db = usable_bounds(display_for_window(state->window));
  w = std::min(WINDOW_WIDTH,  (int)(db.w * 0.85f));
  h = std::min(WINDOW_HEIGHT, (int)(db.h * 0.85f));
  clamp_size_to_bounds(w, h, db, WINDOW_MARGIN);
  x = db.x + (db.w - w) / 2;
  y = db.y + (db.h - h) / 2;
}

} // namespace

// =============================================================================
//  Display bounds
// =============================================================================

SDL_Rect get_display_bounds(AppState* state) {
  return usable_bounds(display_for_window(state->window));
}

bool window_is_maximized(AppState* state) {
  return state->win_maximized;
}

void init_window_geometry(AppState* state) {
  SDL_Rect db = get_display_bounds(state);

  int w = std::min(WINDOW_WIDTH,  (int)(db.w * 0.85f));
  int h = std::min(WINDOW_HEIGHT, (int)(db.h * 0.85f));
  clamp_size_to_bounds(w, h, db, WINDOW_MARGIN);

  int x = db.x + (db.w - w) / 2;
  int y = db.y + (db.h - h) / 2;

  SDL_SetWindowPosition(state->window, x, y);
  SDL_SetWindowSize(state->window, w, h);

  // Read back what the WM actually applied (may differ on Wayland).
  read_window_rect(state, x, y, w, h);
  clamp_size_to_bounds(w, h, db, WINDOW_MARGIN);
  clamp_position_to_bounds(x, y, w, h, db);

  state->win_restore_x = x;
  state->win_restore_y = y;
  state->win_restore_w = w;
  state->win_restore_h = h;
  state->win_maximized = false;
}

// =============================================================================
//  Snap zones
// =============================================================================

SnapZone compute_snap_zone(AppState* state) {
  (void)state;
  constexpr int SNAP_EDGE = 15;
  SDL_Rect db = display_bounds_at_mouse();
  float gx = 0.f, gy = 0.f;
  SDL_GetGlobalMouseState(&gx, &gy);
  int mx = (int)gx, my = (int)gy;

  if (my <= db.y + SNAP_EDGE) return SnapZone::TOP;
  if (mx <= db.x + SNAP_EDGE) return SnapZone::LEFT;
  if (mx >= db.x + db.w - 1 - SNAP_EDGE) return SnapZone::RIGHT;
  return SnapZone::NONE;
}

SDL_Rect snap_target_rect(AppState* state, SnapZone zone) {
  (void)state;
  SDL_Rect db = display_bounds_at_mouse();
  switch (zone) {
    case SnapZone::TOP:
      return db;
    case SnapZone::LEFT:
      return { db.x, db.y, db.w / 2, db.h };
    case SnapZone::RIGHT:
      return { db.x + db.w / 2, db.y, db.w - db.w / 2, db.h };
    default:
      return {};
  }
}

// =============================================================================
//  Maximize / restore — `win_maximized` is the single source of truth
// =============================================================================

void save_window_restore_rect(AppState* state) {
  int x = 0, y = 0, w = 0, h = 0;
  read_window_rect(state, x, y, w, h);

  SDL_Rect db = get_display_bounds(state);
  if (rect_matches_display(x, y, w, h, db))
    return;  // keep existing restore rect; don't save full-screen geometry

  clamp_size_to_bounds(w, h, db, WINDOW_MARGIN);
  clamp_position_to_bounds(x, y, w, h, db);

  state->win_restore_x = x;
  state->win_restore_y = y;
  state->win_restore_w = w;
  state->win_restore_h = h;
}

void maximize_window_to_display(AppState* state) {
  if (!state->win_maximized)
    save_window_restore_rect(state);

  SDL_Rect db = get_display_bounds(state);
  SDL_SetWindowPosition(state->window, db.x, db.y);
  SDL_SetWindowSize(state->window, db.w, db.h);
  state->win_maximized = true;
}

void restore_window(AppState* state) {
  if (!state->win_maximized) return;

  SDL_Rect db = get_display_bounds(state);
  int x = state->win_restore_x;
  int y = state->win_restore_y;
  int w = state->win_restore_w;
  int h = state->win_restore_h;

  if (w <= 0 || h <= 0 || rect_matches_display(x, y, w, h, db))
    default_windowed_rect(state, x, y, w, h);

  clamp_size_to_bounds(w, h, db, WINDOW_MARGIN);
  clamp_position_to_bounds(x, y, w, h, db);

  SDL_SetWindowPosition(state->window, x, y);
  SDL_SetWindowSize(state->window, w, h);
  state->win_maximized = false;
}

void toggle_window_maximize(AppState* state) {
  if (state->win_maximized)
    restore_window(state);
  else
    maximize_window_to_display(state);
}

// =============================================================================
//  Snap drag
// =============================================================================

void begin_snap_drag(AppState* state, float mx, float my) {
  if (state->win_maximized) {
    int new_w = state->win_restore_w;
    int new_h = state->win_restore_h;
    if (new_w <= 0 || new_h <= 0) {
      int nx = 0, ny = 0;
      default_windowed_rect(state, nx, ny, new_w, new_h);
    }

    float rel_x = (mx > 0.f && state->layout.win_w > 0.f)
                  ? (mx / state->layout.win_w) : 0.5f;

    SDL_Rect db = get_display_bounds(state);
    clamp_size_to_bounds(new_w, new_h, db, WINDOW_MARGIN);

    float gx = 0.f, gy = 0.f;
    SDL_GetGlobalMouseState(&gx, &gy);
    int new_x = (int)gx - (int)(rel_x * new_w);
    int new_y = (int)gy - (int)my;
    clamp_position_to_bounds(new_x, new_y, new_w, new_h, db);

    SDL_SetWindowPosition(state->window, new_x, new_y);
    SDL_SetWindowSize(state->window, new_w, new_h);
    state->win_maximized = false;
  }

  read_window_rect(state, state->snap_pre_x, state->snap_pre_y,
                   state->snap_pre_w, state->snap_pre_h);
  state->snap_drag_active = true;
  state->snap_zone = SnapZone::NONE;
}

void apply_snap(AppState* state, SnapZone zone) {
  if (zone == SnapZone::NONE) return;
  SDL_Rect target = snap_target_rect(state, zone);
  if (target.w <= 0 || target.h <= 0) return;

  if (zone == SnapZone::TOP) {
    if (!state->win_maximized) {
      if (state->snap_pre_w > 0 && state->snap_pre_h > 0) {
        SDL_Rect db = get_display_bounds(state);
        if (!rect_matches_display(state->snap_pre_x, state->snap_pre_y,
                                  state->snap_pre_w, state->snap_pre_h, db)) {
          state->win_restore_x = state->snap_pre_x;
          state->win_restore_y = state->snap_pre_y;
          state->win_restore_w = state->snap_pre_w;
          state->win_restore_h = state->snap_pre_h;
        } else {
          save_window_restore_rect(state);
        }
      } else {
        save_window_restore_rect(state);
      }
    }
    SDL_SetWindowPosition(state->window, target.x, target.y);
    SDL_SetWindowSize(state->window, target.w, target.h);
    state->win_maximized = true;
  } else {
    if (state->snap_pre_w > 0 && state->snap_pre_h > 0) {
      state->win_restore_x = state->snap_pre_x;
      state->win_restore_y = state->snap_pre_y;
      state->win_restore_w = state->snap_pre_w;
      state->win_restore_h = state->snap_pre_h;
    }
    SDL_SetWindowPosition(state->window, target.x, target.y);
    SDL_SetWindowSize(state->window, target.w, target.h);
    state->win_maximized = false;
  }
}

void update_snap_during_drag(AppState* state) {
  if (!state->snap_drag_active) return;
  float gx = 0.f, gy = 0.f;
  Uint32 buttons = SDL_GetGlobalMouseState(&gx, &gy);
  if (!(buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))) {
    if (state->snap_zone != SnapZone::NONE)
      apply_snap(state, state->snap_zone);
    state->snap_drag_active = false;
    state->snap_zone = SnapZone::NONE;
    return;
  }
  state->snap_zone = compute_snap_zone(state);
}

void draw_snap_overlay(AppState* state) {
  if (!state->snap_drag_active || state->snap_zone == SnapZone::NONE) return;

  SDL_Renderer* r = state->renderer;
  SDL_Rect target = snap_target_rect(state, state->snap_zone);

  float W = state->layout.win_w;
  float H = state->layout.win_h;
  SDL_Color glow_col{60, 130, 246, 220};

  if (state->snap_zone == SnapZone::TOP) {
    ui_fill_rect(r, 0.f, 0.f, W, 4.f, glow_col);
  } else if (state->snap_zone == SnapZone::LEFT) {
    ui_fill_rect(r, 0.f, 0.f, 4.f, H, glow_col);
  } else if (state->snap_zone == SnapZone::RIGHT) {
    ui_fill_rect(r, W - 4.f, 0.f, 4.f, H, glow_col);
  }

  int wx = 0, wy = 0;
  SDL_GetWindowPosition(state->window, &wx, &wy);
  float ox = (float)(target.x - wx);
  float oy = (float)(target.y - wy);
  ui_fill_rect(r, ox, oy, (float)target.w, (float)target.h, {60, 130, 246, 45});
  ui_draw_rect(r, ox, oy, (float)target.w, (float)target.h, {90, 160, 255, 180});
}
