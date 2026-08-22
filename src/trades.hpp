#pragma once
#include "app_state.hpp"

// =============================================================================
//  Trading Platform — Recent Trades Module
// =============================================================================

// Background thread: polls recent trades for the active symbol every 2 s
void fetch_trades(AppState* state);

// Render the trades panel inside the right area
void draw_trades(AppState* state, float x, float y, float w, float h);
