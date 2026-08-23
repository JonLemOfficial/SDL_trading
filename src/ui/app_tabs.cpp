#include "ui/app_tabs.hpp"
#include "ui/ui_utils.hpp"

void draw_app_tabs(AppState* state) {
  SDL_Renderer* r   = state->renderer;
  const ThemeColors& tc = state->theme_colors;
  float W   = state->layout.win_w;
  float TH  = AppLayout::TITLEBAR_H;
  float ATH = AppLayout::APPTAB_H;
  float tab_y = TH;

  static const char* TAB_LABELS[] = {
    "MARKETS",
    "TRADES",
    "BOTS",
    "STRATEGIES",
    "BACKTESTING",
    "EXCHANGES",
    "ACCOUNT",
    "CONFIGURATION"
  };
  constexpr int NUM_APP_TABS = 8;
  float tab_w = W / (float) NUM_APP_TABS;

  for ( int i = 0; i < NUM_APP_TABS; i++ ) {
    float tx = i * tab_w;
    bool active = ((int)state->active_app_tab == i);
    SDL_Color bg  = active ? tc.bg_tab_active : tc.bg_tab_idle;
    SDL_Color txt = active ? tc.text_primary  : tc.text_muted;
    bool tab_hovered = point_in(state->mouse_x, state->mouse_y, tx, tab_y, tab_w, ATH);
    ui_fill_rect(r, tx, tab_y, tab_w, ATH, tab_hovered ? tc.bg_btn_idle : bg);
    ui_draw_rect(r, tx, tab_y, tab_w, ATH, tc.border);
    ui_draw_text_centered(r, state->font_sm, tx, tab_y, tab_w, ATH, TAB_LABELS[i], txt);
  }
  ui_draw_line(r, 0.f, tab_y + ATH - 1.f, W, tab_y + ATH - 1.f, tc.border_accent);
}

int hit_app_tab(AppState* state, float mx, float my) {
  float TH  = AppLayout::TITLEBAR_H;
  float ATH = AppLayout::APPTAB_H;
  float W   = state->layout.win_w;
  if (my < TH || my >= TH + ATH) return -1;
  float tab_w = W / 8.f;
  int idx = (int)(mx / tab_w);
  if (idx >= 0 && idx < 8) return idx;
  return -1;
}
