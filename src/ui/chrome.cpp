#include "ui/chrome.hpp"

#include "ui/ui_utils.hpp"

#include <chrono>
#include <string>

void draw_dividers(AppState* state) {
  SDL_Renderer* r = state->renderer;
  const ThemeColors& tc = state->theme_colors;
  constexpr float DIV = 4.f;

  float vdiv_x = state->layout.chart_w;
  float cy     = state->layout.content_y;
  float ch     = state->layout.content_h;
  ui_fill_rect(r, vdiv_x, cy, DIV, ch, tc.bg_header);

  float hdiv_y = state->layout.table_y + state->layout.table_h;
  float rx     = state->layout.table_x;
  float rw     = state->layout.table_w;
  ui_fill_rect(r, rx, hdiv_y, rw, DIV, tc.bg_header);
}

void draw_notification(AppState* state) {
  if (state->notification_text.empty()) return;
  int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  int64_t elapsed = now_ms - state->notification_ts;
  if (elapsed > 4000) {
    state->notification_text.clear();
    return;
  }
  Uint8 alpha = (elapsed < 3000) ? 230 : (Uint8)(230 * (1.f - (elapsed - 3000) / 1000.f));
  SDL_Renderer* r = state->renderer;
  const ThemeColors& tc = state->theme_colors;

  float nw = 400.f, nh = 60.f;
  float nx = ((float)state->layout.win_w - nw) * 0.5f;
  float ny = AppLayout::TITLEBAR_H + AppLayout::APPTAB_H + 20.f;

  SDL_Color nbg = tc.notification_bg;
  nbg.a = alpha;
  ui_fill_rect(r, nx, ny, nw, nh, nbg);
  SDL_Color nbrd = tc.border_accent;
  nbrd.a = alpha;
  ui_draw_rect(r, nx, ny, nw, nh, nbrd);

  SDL_Color ntxt = tc.text_primary;
  ntxt.a = alpha;
  std::string first_line = state->notification_text;
  auto nl = first_line.find('\n');
  if (nl != std::string::npos) first_line = first_line.substr(0, nl);
  ui_draw_text_centered(r, state->font_sm, nx, ny, nw, nh,
                        first_line.c_str(), ntxt);
}

void draw_placeholder(AppState* state, const char* label) {
  SDL_Renderer* r   = state->renderer;
  const ThemeColors& tc = state->theme_colors;
  float W  = state->layout.win_w;
  float H  = state->layout.content_h;
  float y0 = state->layout.content_y;
  ui_fill_rect(r, 0.f, y0, W, H, tc.bg_window);

  int tw = ui_text_width(state->font_lg, label);
  float tx = (W - (float)tw) * 0.5f;
  float ty = y0 + H * 0.5f - 10.f;
  ui_draw_text(r, state->font_lg, tx, ty, label, tc.text_muted);
}
