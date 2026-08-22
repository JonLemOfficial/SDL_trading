#pragma once
#include "app_state.hpp"

// =============================================================================
//  Trading Platform — Order Book Module
// =============================================================================

// Background thread: polls order book for the active chart_symbol every 2 s
void fetch_orderbook(AppState* state);

// Render the order-book panel inside the right area
void draw_orderbook(AppState* state, float x, float y, float w, float h);
