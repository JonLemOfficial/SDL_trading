#pragma once
#include "core/app_state.hpp"

// =============================================================================
//  Chart Module — candlestick panel + Binance kline fetch thread
// =============================================================================

/// Timeframe button metrics (shared by draw + hit-test).
struct ChartTfBtnLayout {
  static constexpr float BTN_W       = 32.f;
  static constexpr float BTN_H       = 20.f;
  static constexpr float BTN_SPACING = 0.f;
};

/// Returns the chart panel origin/size (below toolbox, excluding outer padding).
void chart_panel_rect(AppState* state, float& cx, float& cy, float& cw, float& ch);

/// Returns screen rect for timeframe button `index` (0 .. NUM_TIMEFRAMES-1).
void chart_timeframe_btn_rect(AppState* state, int index,
                              float& bx, float& by, float& bw, float& bh);

/// Returns clicked timeframe index, or -1 if none.
int hit_chart_timeframe(AppState* state, float mx, float my);

/// Background thread: fetches OHLCV candles for `state->chart_symbol`.
void fetch_candles(AppState* state);

/// Renders the candlestick chart, price axis, timeframes, and user drawings.
void draw_chart(AppState* state);

