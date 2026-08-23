#pragma once
#include "core/app_state.hpp"
#include <string>

// =============================================================================
//  Trading Platform — Alerts Module
// =============================================================================

// Background thread: evaluates alerts every 5 s against latest pair data
void alerts_engine(AppState* state);

// Render the alerts list panel
void draw_alerts(AppState* state, float x, float y, float w, float h);

// Render the alert create/edit modal (overlay, full-window)
void draw_alert_modal(AppState* state);
bool handle_alert_modal_click(AppState* state, float mx, float my);
bool update_alert_modal_cursor(AppState* state, float mx, float my);

// Text input routing for the modal (call from SDL_EVENT_TEXT_INPUT)
void handle_alert_modal_text(AppState* state, const char* text);
// Backspace routing for the modal (call from SDL_EVENT_KEY_DOWN SDLK_BACKSPACE)
void handle_alert_modal_backspace(AppState* state);
// Commits the threshold string buffer to the draft float value
void alert_commit_threshold(AppState* state);

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

