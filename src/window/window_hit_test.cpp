#include "window/window_hit_test.hpp"

ResizeDir get_resize_dir(AppState* state, float mx, float my) {
  float W   = state->layout.win_w;
  float H   = state->layout.win_h;
  float RB  = AppLayout::RESIZE_BORDER;
  float TH  = AppLayout::TITLEBAR_H;

  bool on_left   = mx < RB;
  bool on_right  = mx > W - RB;
  bool on_top    = my < RB;
  bool on_bottom = my > H - RB;

  if (my >= RB && my < TH) return ResizeDir::NONE;

  if (on_top    && on_left)  return ResizeDir::NW;
  if (on_top    && on_right) return ResizeDir::NE;
  if (on_bottom && on_left)  return ResizeDir::SW;
  if (on_bottom && on_right) return ResizeDir::SE;
  if (on_top)                return ResizeDir::N;
  if (on_bottom)             return ResizeDir::S;
  if (on_left)               return ResizeDir::W;
  if (on_right)              return ResizeDir::E;
  return ResizeDir::NONE;
}

SDL_Cursor* cursor_for_resize(AppState* state, ResizeDir d) {
  switch (d) {
    case ResizeDir::N:
    case ResizeDir::S:  return state->cursor_resize_ns;
    case ResizeDir::E:
    case ResizeDir::W:  return state->cursor_resize_ew;
    case ResizeDir::NW:
    case ResizeDir::SE: return state->cursor_resize_nwse;
    case ResizeDir::NE:
    case ResizeDir::SW: return state->cursor_resize_nesw;
    default:            return state->cursor_arrow;
  }
}

SDL_HitTestResult SDLCALL hit_test_callback(SDL_Window* win, const SDL_Point* area, void* data) {
  (void)win;
  AppState* state = (AppState*)data;
  float mx = (float)area->x;
  float my = (float)area->y;

  ResizeDir rd = get_resize_dir(state, mx, my);
  if (rd != ResizeDir::NONE) {
    switch (rd) {
      case ResizeDir::NW: return SDL_HITTEST_RESIZE_TOPLEFT;
      case ResizeDir::N:  return SDL_HITTEST_RESIZE_TOP;
      case ResizeDir::NE: return SDL_HITTEST_RESIZE_TOPRIGHT;
      case ResizeDir::E:  return SDL_HITTEST_RESIZE_RIGHT;
      case ResizeDir::SE: return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
      case ResizeDir::S:  return SDL_HITTEST_RESIZE_BOTTOM;
      case ResizeDir::SW: return SDL_HITTEST_RESIZE_BOTTOMLEFT;
      case ResizeDir::W:  return SDL_HITTEST_RESIZE_LEFT;
      default: break;
    }
  }

  if (my < AppLayout::TITLEBAR_H) {
    if (hit_close_btn(state, mx, my) ||
        hit_max_btn(state, mx, my) ||
        hit_min_btn(state, mx, my)) {
      return SDL_HITTEST_NORMAL;
    }
    return SDL_HITTEST_DRAGGABLE;
  }

  return SDL_HITTEST_NORMAL;
}
