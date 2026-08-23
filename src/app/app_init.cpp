#include <SDL3_ttf/SDL_ttf.h>
#include <curl/curl.h>
#include <thread>

#include "app/app_init.hpp"

#include "core/app_state.hpp"
#include "core/constants.hpp"
#include "modules/alerts.hpp"
#include "modules/chart.hpp"
#include "modules/market_table.hpp"
#include "modules/orderbook.hpp"
#include "modules/trades.hpp"
#include "services/account.hpp"
#include "services/favorites.hpp"
#include "ui/theme.hpp"
#include "window/window_hit_test.hpp"
#include "window/window_snap.hpp"

SDL_AppResult app_init(void** appstate, int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  if (!SDL_Init(SDL_INIT_VIDEO))             return SDL_APP_FAILURE;
  if (!TTF_Init())                           return SDL_APP_FAILURE;
  if (curl_global_init(CURL_GLOBAL_DEFAULT)) return SDL_APP_FAILURE;

  AppState* state = new AppState();
  *appstate = state;

  state->theme        = Theme::SYSTEM;
  state->theme_colors = get_theme(state->theme);

  state->window = SDL_CreateWindow(
    "SDL Trading",
    (int)state->layout.win_w, (int)state->layout.win_h,
    SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE
  );

  if (!state->window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_SetWindowHitTest(state->window, hit_test_callback, state);

  init_window_geometry(state);

  state->renderer = SDL_CreateRenderer(state->window, nullptr);
  if (!state->renderer) {
    SDL_Log("Couldn't create renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);

  state->font    = TTF_OpenFont(FONT_PATH, 13);
  state->font_sm = TTF_OpenFont(FONT_PATH, 11);
  state->font_lg = TTF_OpenFont(FONT_PATH, 16);
  if (!state->font || !state->font_sm || !state->font_lg) {
    SDL_Log("Error: Could not open font at %s", FONT_PATH);
    return SDL_APP_FAILURE;
  }

  state->font_icon    = TTF_OpenFont(ICON_FONT_PATH, 18);
  state->font_icon_sm = TTF_OpenFont(ICON_FONT_PATH, 15);
  if (!state->font_icon || !state->font_icon_sm) {
    SDL_Log("Warning: Icon font not found at %s, falling back to text glyphs", ICON_FONT_PATH);
  }

  fav_load(state->favorites, FAVORITES_FILE);
  alerts_load(state->alerts, ALERTS_FILE);

  state->candle_thread  = std::thread(fetch_candles, state);
  state->pairs_thread   = std::thread(fetch_pairs, state);
  state->ob_thread      = std::thread(fetch_orderbook, state);
  state->trades_thread  = std::thread(fetch_trades, state);
  state->alerts_thread  = std::thread(alerts_engine, state);
  state->account_thread = std::thread(mock_account_thread, state);

  state->cursor_arrow       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
  state->cursor_hand        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
  state->cursor_cross       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
  state->cursor_resize_ns   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
  state->cursor_resize_ew   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
  state->cursor_resize_nwse = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
  state->cursor_resize_nesw = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);
  state->cursor_move        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);

  return SDL_APP_CONTINUE;
}
