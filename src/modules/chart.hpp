#pragma once
#include "core/app_state.hpp"

// =============================================================================
//  Chart Module — candlestick panel + Binance kline fetch thread
// =============================================================================

/// Background thread: fetches OHLCV candles for `state->chart_symbol`.
void fetch_candles(AppState* state);

/// Renders the candlestick chart, price axis, timeframes, and user drawings.
void draw_chart(AppState* state);

