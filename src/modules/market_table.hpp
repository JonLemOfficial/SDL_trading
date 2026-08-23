#pragma once
#include "core/app_state.hpp"
#include <vector>

// =============================================================================
//  Trading Platform — Market Table Module
//  Covers SPOT / FUTURES / FAVORITES tabs with search filter.
// =============================================================================

// Background thread: fetches 24-hr ticker data for SPOT and FUTURES
void fetch_pairs(AppState* state);

// Render the market table panel
void draw_table(AppState* state);

// Returns sorted + filtered pair list for the active tab
std::vector<PairInfo> get_sorted_filtered_pairs(AppState* state);

// Hit-test helpers (called from event handler)
int  hit_tab         (AppState* state, float mx, float my);
int  hit_col_header  (AppState* state, float mx, float my);
int  hit_table_row   (float mx, float my, AppState* state,
                      const std::vector<PairInfo>& sorted);
bool hit_search_bar  (AppState* state, float mx, float my);
bool hit_fav_star    (AppState* state, float mx, float my, int row_idx);
bool hit_theme_button(AppState* state, float mx, float my);
