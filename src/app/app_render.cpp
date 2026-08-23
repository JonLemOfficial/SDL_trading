#include "app/app_render.hpp"

#include "core/app_state.hpp"
#include "core/layout.hpp"
#include "modules/alerts.hpp"
#include "modules/chart.hpp"
#include "modules/market_table.hpp"
#include "modules/right_panel.hpp"
#include "ui/app_tabs.hpp"
#include "ui/chrome.hpp"
#include "ui/toolbox.hpp"
#include "window/titlebar.hpp"
#include "window/window_snap.hpp"

SDL_AppResult app_iterate(void* appstate) {
  AppState* state = (AppState*)appstate;
  update_snap_during_drag(state);
  update_layout(state);
  SDL_Renderer* r = state->renderer;
  const ThemeColors& tc = state->theme_colors;

  SDL_SetRenderDrawColor(r, tc.bg_window.r, tc.bg_window.g, tc.bg_window.b, 255);
  SDL_RenderClear(r);

  draw_titlebar(state);
  draw_app_tabs(state);

  switch (state->active_app_tab) {
    case AppTab::MARKETS:
      draw_chart(state);
      draw_toolbox(state);
      draw_right_panel(state);
      draw_table(state);
      draw_dividers(state);
      break;
    case AppTab::TRADES:
      draw_placeholder(state, "Trades — Coming Soon");
      break;
    case AppTab::BOTS:
      draw_placeholder(state, "Bots — Coming Soon");
      break;
    case AppTab::STRATEGIES:
      draw_placeholder(state, "Strategies — Coming Soon");
      break;
    case AppTab::BACKTESTING:
      draw_placeholder(state, "Backtesting — Coming Soon");
      break;
    case AppTab::EXCHANGES:
      draw_placeholder(state, "Exchanges — Coming Soon");
      break;
    case AppTab::ACCOUNT:
      draw_placeholder(state, "Account — Coming Soon");
      break;
    case AppTab::CONFIGURATION:
      draw_placeholder(state, "Configuration — Coming Soon");
      break;
  }

  draw_notification(state);

  if (state->alert_modal_open)
    draw_alert_modal(state);

  draw_snap_overlay(state);

  SDL_RenderPresent(r);
  return SDL_APP_CONTINUE;
}
