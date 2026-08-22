#pragma once
#include "app_state.hpp"

// =============================================================================
//  Trading Platform — Right Panel
//  Hosts the ORDER BOOK | TRADES | ALERTS tabs for the selected asset.
// =============================================================================

void draw_right_panel(AppState* state);

// Hit-tests for the right-panel tabs
int  hit_right_tab(AppState* state, float mx, float my);
// Returns the x, y, w, h of the right panel content area
void right_panel_content_rect(AppState* state, float& x, float& y, float& w, float& h);
