#pragma once
#include "app_state.hpp"
#include <string>

// =============================================================================
//  Trading Platform — Alerts Module
//
//  Alert types:
//    PRICE_ABOVE       – triggers when price > threshold
//    PRICE_BELOW       – triggers when price < threshold
//    PRICE_REACHES     – triggers when price crosses threshold (either dir)
//    CHANGE_24H_ABOVE  – triggers when 24h % > threshold
//    CHANGE_24H_BELOW  – triggers when 24h % < threshold
//    VOLUME_SPIKE      – triggers when volume_24h > threshold (USD)
//    VOLATILITY        – triggers when (H-L)/L*100 > threshold %
//
//  Frequency: ONCE (auto-disables) | ALWAYS (re-arms each check cycle)
//  Channels:  in-app banner + optional Telegram notification
// =============================================================================

// Background thread: evaluates alerts every 5 s against latest pair data
void alerts_engine(AppState* state);

// Render the alerts list panel
void draw_alerts(AppState* state, float x, float y, float w, float h);

// Render the alert create/edit modal (overlay, full-window)
void draw_alert_modal(AppState* state);
bool handle_alert_modal_click(AppState* state, float mx, float my);
bool update_alert_modal_cursor(AppState* state, float mx, float my);

// Helper: returns a human-readable label for an AlertType
const char* alert_type_label(AlertType t);
const char* alert_freq_label(AlertFrequency f);

// Persistence
void alerts_save(const std::vector<Alert>& alerts, const std::string& path = "alerts.json");
void alerts_load(std::vector<Alert>& alerts, const std::string& path = "alerts.json");

// Generate a simple unique ID
std::string alerts_new_id();

// Hit-test
bool hit_alerts_add_button(float mx, float my, float x, float y, float w);
int  hit_alert_row(float mx, float my, float x, float y, float w, float h, int count);
bool hit_alert_del_btn(float mx, float my, float x, float row_y, float w);
bool hit_alert_toggle_btn(float mx, float my, float x, float row_y);
