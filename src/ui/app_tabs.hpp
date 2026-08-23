#pragma once

#include "core/app_state.hpp"

// =============================================================================
//  App Tabs — global navigation bar (MARKETS, TRADES, …)
// =============================================================================

/// Draws the horizontal app-tab bar below the titlebar.
void draw_app_tabs(AppState* state);

/// Returns the tab index under (mx, my), or -1 if none.
int hit_app_tab(AppState* state, float mx, float my);
