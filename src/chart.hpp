#pragma once
#include "app_state.hpp"

// =============================================================================
//  Trading Platform — Chart Module
// =============================================================================

// Background thread: fetches candles and updates state->candles
void fetch_candles(AppState* state);

// Render the candlestick chart panel (left side)
void draw_chart(AppState* state);
