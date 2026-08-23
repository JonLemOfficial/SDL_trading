#include "core/layout.hpp"

#include <algorithm>

// =============================================================================
//  Layout computation
// =============================================================================

void update_layout(AppState* state) {
  int iw = 0, ih = 0;
  SDL_GetWindowSize(state->window, &iw, &ih);
  float W = (float)iw;
  float H = (float)ih;
  constexpr float DIV = 4.f;

  state->layout.win_w = W;
  state->layout.win_h = H;

  float cy_off = AppLayout::TITLEBAR_H + AppLayout::APPTAB_H;
  float c_h    = H - cy_off;

  state->layout.content_y = cy_off;
  state->layout.content_h = c_h;

  float split_x = W * std::max(0.40f, std::min(0.85f, state->layout.split_ratio));

  state->layout.chart_x = 0.f;
  state->layout.chart_y = cy_off;
  state->layout.chart_w = split_x;
  state->layout.chart_h = c_h;

  state->layout.toolbox_x = 0.f;
  state->layout.toolbox_y = cy_off;
  state->layout.toolbox_w = AppLayout::TOOLBOX_W;
  state->layout.toolbox_h = c_h;

  float right_x = split_x + DIV;
  float right_w = W - right_x;

  float vsplit_y = cy_off + c_h * std::max(0.20f, std::min(0.80f, state->layout.vsplit_ratio));

  state->layout.table_x = right_x;
  state->layout.table_y = cy_off;
  state->layout.table_w = right_w;
  state->layout.table_h = vsplit_y - cy_off;

  state->layout.right_tabs_x = right_x;
  state->layout.right_tabs_y = vsplit_y + DIV;
  state->layout.right_tabs_w = right_w;
  state->layout.right_tabs_h = H - vsplit_y - DIV;
}
