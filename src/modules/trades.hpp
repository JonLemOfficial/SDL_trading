#pragma once
#include "core/app_state.hpp"

// =============================================================================
//  Trading Platform — Recent Trades Module
// =============================================================================

/// Background thread: polls Binance recent trades for `state->chart_symbol`.
void fetch_trades(AppState* state);

/// Renders the recent trades list (price, qty, time).
void draw_trades(AppState* state, float x, float y, float w, float h);
