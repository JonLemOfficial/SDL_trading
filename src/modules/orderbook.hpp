#pragma once
#include "core/app_state.hpp"

// =============================================================================
//  Trading Platform — Order Book Module
// =============================================================================

/// Background thread: polls Binance order book depth for `state->chart_symbol`.
void fetch_orderbook(AppState* state);

/// Renders the bid/ask ladder with cumulative depth bars.
void draw_orderbook(AppState* state, float x, float y, float w, float h);
