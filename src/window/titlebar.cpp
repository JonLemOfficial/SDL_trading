#include "window/titlebar.hpp"

#include "core/constants.hpp"
#include "ui/ui_utils.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

bool hit_close_btn(AppState* state, float mx, float my) {
  float W = state->layout.win_w;
  float H = AppLayout::TITLEBAR_H;
  return point_in(mx, my, W - TITLEBAR_BTN_W, 0.f, TITLEBAR_BTN_W, H);
}

bool hit_max_btn(AppState* state, float mx, float my) {
  float W = state->layout.win_w;
  float H = AppLayout::TITLEBAR_H;
  return point_in(mx, my, W - TITLEBAR_BTN_W * 2.f, 0.f, TITLEBAR_BTN_W, H);
}

bool hit_min_btn(AppState* state, float mx, float my) {
  float W = state->layout.win_w;
  float H = AppLayout::TITLEBAR_H;
  return point_in(mx, my, W - TITLEBAR_BTN_W * 3.f, 0.f, TITLEBAR_BTN_W, H);
}

void draw_titlebar(AppState* state) {
  SDL_Renderer* r   = state->renderer;
  const ThemeColors& tc = state->theme_colors;
  float W  = state->layout.win_w;
  float TH = AppLayout::TITLEBAR_H;

  ui_fill_rect(r, 0.f, 0.f, W, TH, tc.bg_header);
  // ui_draw_line(r, 0.f, TH - 1.f, W, TH - 1.f, tc.border);

  float price = 0.f;
  float change_24h = 0.f;
  {
    std::lock_guard<std::mutex> lock(state->pairs_mtx);
    auto it = std::find_if(state->pairs_spot.begin(), state->pairs_spot.end(),
      [&](const PairInfo& p){ return p.symbol == state->chart_symbol; });
    if (it != state->pairs_spot.end()) { price = it->price; change_24h = it->change_24h; }
    else {
      auto it2 = std::find_if(state->pairs_futures.begin(), state->pairs_futures.end(),
        [&](const PairInfo& p){ return p.symbol == state->chart_symbol; });
      if (it2 != state->pairs_futures.end()) { price = it2->price; change_24h = it2->change_24h; }
    }
  }

  int active_alerts = 0;
  {
    std::lock_guard<std::mutex> lock(state->alerts_mtx);
    for (const auto& a : state->alerts) if (a.enabled) active_alerts++;
  }

  float bal, pnl;
  int open, pending;
  {
    std::lock_guard<std::mutex> lock(state->account_mtx);
    bal = state->account.balance;
    pnl = state->account.pnl;
    open = state->account.open_trades;
    pending = state->account.pending_trades;
  }

  float tx = 10.f;
  float ty = (TH - 12.f) * 0.5f;

  auto draw_seg = [&](const std::string& text, SDL_Color color) {
    if (text.empty()) return;
    ui_draw_text(r, state->font_sm, tx, ty, text.c_str(), color);
    tx += ui_text_width(state->font_sm, text.c_str());
  };

  char buf[64];

  draw_seg("SDL Trading — Binance | ", tc.text_header);
  draw_seg("Asset: ", tc.text_muted);
  draw_seg(state->chart_symbol.empty() ? "None" : state->chart_symbol, tc.text_primary);
  draw_seg(" | Price: ", tc.text_muted);
  snprintf(buf, sizeof(buf), price > 1 ? "%.3f USD" : "%.6f USD", price);
  draw_seg(buf, tc.text_primary);
  draw_seg(" | 24h: ", tc.text_muted);
  snprintf(buf, sizeof(buf), "%+.2f%%", change_24h);
  draw_seg(buf, (change_24h >= 0.f) ? tc.bull : tc.bear);
  draw_seg(" | ", tc.text_muted);
  snprintf(buf, sizeof(buf), "%d", active_alerts);
  draw_seg(buf, tc.text_primary);
  draw_seg(" Alerts | ", tc.text_muted);
  snprintf(buf, sizeof(buf), "%d", pending);
  draw_seg(buf, tc.text_primary);
  draw_seg(" Pending trades | ", tc.text_muted);
  snprintf(buf, sizeof(buf), "%d", open);
  draw_seg(buf, tc.text_primary);
  draw_seg(" Open Trades | PNL: ", tc.text_muted);
  snprintf(buf, sizeof(buf), "%+.2f", pnl);
  draw_seg(buf, (pnl >= 0.f) ? tc.bull : tc.bear);
  draw_seg(" | Disp: ", tc.text_muted);
  snprintf(buf, sizeof(buf), "%.2f USDT", bal);
  draw_seg(buf, tc.text_primary);

  float min_x = W - TITLEBAR_BTN_W * 3.f;
  bool  min_h = point_in(state->mouse_x, state->mouse_y, min_x, 0.f, TITLEBAR_BTN_W, TH);
  ui_fill_rect(r, min_x, 0.f, TITLEBAR_BTN_W, TH, min_h ? tc.bg_btn_idle : tc.bg_header);
  if (state->font_icon_sm) {
    ui_draw_text_centered(r, state->font_icon_sm, min_x, 0.f, TITLEBAR_BTN_W, TH,
                          ICON_MINIMIZE, tc.text_muted);
  } else {
    ui_draw_line(r, min_x + 15.f, TH * 0.5f, min_x + TITLEBAR_BTN_W - 15.f, TH * 0.5f, tc.text_muted);
  }

  float max_x = W - TITLEBAR_BTN_W * 2.f;
  bool  max_h = point_in(state->mouse_x, state->mouse_y, max_x, 0.f, TITLEBAR_BTN_W, TH);
  ui_fill_rect(r, max_x, 0.f, TITLEBAR_BTN_W, TH, max_h ? tc.bg_btn_idle : tc.bg_header);
  if (state->font_icon_sm) {
    const char* max_icon = state->win_maximized ? ICON_RESTORE : ICON_MAXIMIZE;
    ui_draw_text_centered(r, state->font_icon_sm, max_x, 0.f, TITLEBAR_BTN_W, TH,
                          max_icon, tc.text_muted);
  } else {
    float cy2 = TH * 0.5f;
    ui_draw_rect(r, max_x + 13.f, cy2 - 7.f, 14.f, 14.f, tc.text_muted);
  }

  float cls_x = W - TITLEBAR_BTN_W;
  bool  cls_h = point_in(state->mouse_x, state->mouse_y, cls_x, 0.f, TITLEBAR_BTN_W, TH);
  SDL_Color cls_bg = cls_h ? SDL_Color{196, 43, 28, 255} : tc.bg_header;
  ui_fill_rect(r, cls_x, 0.f, TITLEBAR_BTN_W, TH, cls_bg);
  if (state->font_icon) {
    ui_draw_text_centered(r, state->font_icon, cls_x, 0.f, TITLEBAR_BTN_W, TH,
                          ICON_CLOSE, tc.text_primary);
  } else {
    float cy2 = TH * 0.5f, ic = 7.f;
    ui_draw_line(r, cls_x + 14.f, cy2 - ic, cls_x + TITLEBAR_BTN_W - 14.f, cy2 + ic, tc.text_primary);
    ui_draw_line(r, cls_x + TITLEBAR_BTN_W - 14.f, cy2 - ic, cls_x + 14.f, cy2 + ic, tc.text_primary);
  }
}
