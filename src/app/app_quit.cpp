#include "app/app_quit.hpp"

#include "core/app_state.hpp"
#include "core/constants.hpp"
#include "modules/alerts.hpp"
#include "services/favorites.hpp"

#include <SDL3_ttf/SDL_ttf.h>
#include <curl/curl.h>
#include <mutex>
#include <thread>

void app_quit(void* appstate, SDL_AppResult result) {
  (void)result;
  AppState* state = (AppState*)appstate;
  if (!state) return;

  state->running = false;
  auto join = [](std::thread& t) { if (t.joinable()) t.join(); };
  join(state->candle_thread);
  join(state->pairs_thread);
  join(state->ob_thread);
  join(state->trades_thread);
  join(state->alerts_thread);
  join(state->account_thread);

  fav_save(state->favorites, FAVORITES_FILE);
  {
    std::lock_guard<std::mutex> lock(state->alerts_mtx);
    alerts_save(state->alerts, ALERTS_FILE);
  }

  if (state->font_lg)       TTF_CloseFont(state->font_lg);
  if (state->font)          TTF_CloseFont(state->font);
  if (state->font_sm)       TTF_CloseFont(state->font_sm);
  if (state->font_icon)     TTF_CloseFont(state->font_icon);
  if (state->font_icon_sm)  TTF_CloseFont(state->font_icon_sm);
  if (state->renderer)      SDL_DestroyRenderer(state->renderer);
  if (state->window)        SDL_DestroyWindow(state->window);

  if (state->cursor_arrow)       SDL_DestroyCursor(state->cursor_arrow);
  if (state->cursor_hand)        SDL_DestroyCursor(state->cursor_hand);
  if (state->cursor_cross)       SDL_DestroyCursor(state->cursor_cross);
  if (state->cursor_resize_ns)   SDL_DestroyCursor(state->cursor_resize_ns);
  if (state->cursor_resize_ew)   SDL_DestroyCursor(state->cursor_resize_ew);
  if (state->cursor_resize_nwse) SDL_DestroyCursor(state->cursor_resize_nwse);
  if (state->cursor_resize_nesw) SDL_DestroyCursor(state->cursor_resize_nesw);
  if (state->cursor_move)        SDL_DestroyCursor(state->cursor_move);

  delete state;
  TTF_Quit();
  curl_global_cleanup();
  SDL_Quit();
}
