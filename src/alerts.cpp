#include "alerts.hpp"
#include "network.hpp"
#include "ui_utils.hpp"
#include "constants.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <sstream>

using json = nlohmann::json;

// =============================================================================
//  Helpers
// =============================================================================

const char* alert_type_label(AlertType t) {
    switch (t) {
        case AlertType::PRICE_ABOVE:       return "Price rises above";
        case AlertType::PRICE_BELOW:       return "Price falls below";
        case AlertType::PRICE_REACHES:     return "Price reaches";
        case AlertType::CHANGE_24H_ABOVE:  return "24h change > %";
        case AlertType::CHANGE_24H_BELOW:  return "24h change < %";
        case AlertType::VOLUME_SPIKE:      return "Volume spike > $";
        case AlertType::VOLATILITY:        return "Volatility > %";
        default: return "Unknown";
    }
}

const char* alert_freq_label(AlertFrequency f) {
    return (f == AlertFrequency::ONCE) ? "Once" : "Always";
}

std::string alerts_new_id() {
    static std::mt19937_64 rng(std::random_device{}());
    std::ostringstream ss;
    ss << std::hex << rng() << rng();
    return ss.str().substr(0, 16);
}

// =============================================================================
//  Persistence
// =============================================================================

void alerts_save(const std::vector<Alert>& alerts, const std::string& path) {
    json arr = json::array();
    for (const auto& a : alerts) {
        json obj;
        obj["id"]           = a.id;
        obj["symbol"]       = a.symbol;
        obj["type"]         = (int)a.type;
        obj["frequency"]    = (int)a.frequency;
        obj["threshold"]    = a.threshold;
        obj["enabled"]      = a.enabled;
        obj["via_telegram"] = a.via_telegram;
        obj["tg_bot_token"] = a.tg_bot_token;
        obj["tg_chat_id"]   = a.tg_chat_id;
        obj["note"]         = a.note;
        arr.push_back(obj);
    }
    std::ofstream o(path);
    if (o) o << arr.dump(2);
}

void alerts_load(std::vector<Alert>& alerts, const std::string& path) {
    std::ifstream in(path);
    if (!in) return;
    try {
        json arr;
        in >> arr;
        alerts.clear();
        for (auto& obj : arr) {
            Alert a;
            a.id           = obj.value("id", alerts_new_id());
            a.symbol       = obj.value("symbol", "");
            a.type         = (AlertType)obj.value("type", 0);
            a.frequency    = (AlertFrequency)obj.value("frequency", 0);
            a.threshold    = obj.value("threshold", 0.f);
            a.enabled      = obj.value("enabled", true);
            a.via_telegram = obj.value("via_telegram", false);
            a.tg_bot_token = obj.value("tg_bot_token", "");
            a.tg_chat_id   = obj.value("tg_chat_id", "");
            a.note         = obj.value("note", "");
            alerts.push_back(a);
        }
    } catch (...) {}
}

// =============================================================================
//  Alert evaluation
// =============================================================================

static bool evaluate_alert(const Alert& a, const PairInfo* p) {
    if (!p) return false;
    switch (a.type) {
        case AlertType::PRICE_ABOVE:      return p->price > a.threshold;
        case AlertType::PRICE_BELOW:      return p->price < a.threshold;
        case AlertType::PRICE_REACHES:    return std::abs(p->price - a.threshold) / a.threshold < 0.005f;
        case AlertType::CHANGE_24H_ABOVE: return p->change_24h > a.threshold;
        case AlertType::CHANGE_24H_BELOW: return p->change_24h < a.threshold;
        case AlertType::VOLUME_SPIKE:     return p->volume_24h > a.threshold;
        case AlertType::VOLATILITY: {
            if (p->low_24h <= 0.f) return false;
            float vol_pct = (p->high_24h - p->low_24h) / p->low_24h * 100.f;
            return vol_pct > a.threshold;
        }
        default: return false;
    }
}

static const PairInfo* find_pair(AppState* state, const std::string& sym) {
    for (const auto& p : state->pairs_spot)
        if (p.symbol == sym) return &p;
    for (const auto& p : state->pairs_futures)
        if (p.symbol == sym) return &p;
    return nullptr;
}

// =============================================================================
//  Alert engine thread
// =============================================================================

void alerts_engine(AppState* state) {
    while (state->running) {
        // Sleep 5 s in slices
        for (int i = 0; i < 50 && state->running; i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!state->running) break;

        std::lock_guard<std::mutex> pairs_lock(state->pairs_mtx);
        std::lock_guard<std::mutex> alert_lock(state->alerts_mtx);

        for (auto& a : state->alerts) {
            if (!a.enabled) continue;

            const PairInfo* p = find_pair(state, a.symbol);
            bool fired = evaluate_alert(a, p);
            if (!fired) { a.triggered = false; continue; }

            // Already triggered and ALWAYS — skip until it resets
            if (a.triggered && a.frequency == AlertFrequency::ALWAYS) continue;
            // Already triggered and ONCE — already disabled
            if (!a.enabled) continue;

            // Fire!
            a.triggered = true;

            // Build notification text
            std::ostringstream msg;
            msg << "🔔 ALERT [" << a.symbol << "]\n"
                << alert_type_label(a.type) << " " << a.threshold << "\n";
            if (p) msg << "Current: " << fmt_price(p->price)
                       << "  24h: " << fmt_pct(p->change_24h);
            if (!a.note.empty()) msg << "\nNote: " << a.note;
            std::string msg_str = msg.str();

            // In-app notification banner
            {
                state->notification_text = msg_str;
                // Get current time in ms
                state->notification_ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            }

            // Telegram
            if (a.via_telegram && !a.tg_bot_token.empty() && !a.tg_chat_id.empty()) {
                std::string bot = a.tg_bot_token;
                std::string chat = a.tg_chat_id;
                // Fire in a detached thread to avoid blocking engine
                std::thread([bot, chat, msg_str]() {
                    telegram_send(bot, chat, msg_str);
                }).detach();
            }

            // Disable if ONCE
            if (a.frequency == AlertFrequency::ONCE)
                a.enabled = false;
            // For ALWAYS: reset triggered flag next iteration
        }
    }
}

// =============================================================================
//  Hit-test helpers
// =============================================================================

static constexpr float ALERT_ROW_H = 58.f;
static constexpr float ALERT_HDR_H = 22.f;
static constexpr float ALERT_BTN_H = 24.f;

bool hit_alerts_add_button(float mx, float my, float x, float y, float w) {
    float bx = x + w - 84.f;
    float by = y + ALERT_HDR_H + 2.f;
    return point_in(mx, my, bx, by, 80.f, ALERT_BTN_H);
}

int hit_alert_row(float mx, float my, float x, float y, float w, float h, int count) {
    float list_y = y + ALERT_HDR_H + ALERT_BTN_H + 4.f;
    if (!point_in(mx, my, x, list_y, w, h - ALERT_HDR_H - ALERT_BTN_H - 4.f)) return -1;
    int idx = (int)((my - list_y) / ALERT_ROW_H);
    if (idx < 0 || idx >= count) return -1;
    return idx;
}

bool hit_alert_del_btn(float mx, float my, float x, float row_y, float w) {
    return point_in(mx, my, x + w - 24.f, row_y + 4.f, 20.f, 18.f);
}

bool hit_alert_toggle_btn(float mx, float my, float x, float row_y) {
    return point_in(mx, my, x + 4.f, row_y + ALERT_ROW_H - 22.f, 38.f, 18.f);
}

// =============================================================================
//  Alerts list panel
// =============================================================================

void draw_alerts(AppState* state, float x, float y, float w, float h) {
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    ui_fill_rect(r, x, y, w, h, tc.bg_panel);

    // Header + Add button
    ui_fill_rect(r, x, y, w, ALERT_HDR_H, tc.bg_header);
    ui_draw_text(r, state->font_sm, x + 4.f, y + 4.f, "PRICE ALERTS", tc.text_header);

    float bx = x + w - 84.f;
    float by = y + ALERT_HDR_H + 2.f;
    ui_fill_rect(r, bx, by, 80.f, ALERT_BTN_H, tc.bg_btn_active);
    ui_draw_rect(r, bx, by, 80.f, ALERT_BTN_H, tc.border_accent);
    ui_draw_text_centered(r, state->font_sm, bx, by, 80.f, ALERT_BTN_H,
                          "+ New Alert", tc.text_primary);

    std::lock_guard<std::mutex> lock(state->alerts_mtx);
    if (state->alerts.empty()) {
        ui_draw_text_centered(r, state->font_sm, x, y + 60.f, w, h - 60.f,
                              "No alerts configured.\nClick '+ New Alert'.", tc.text_muted);
        return;
    }

    float list_y = by + ALERT_BTN_H + 4.f;
    float max_y  = y + h - 4.f;
    int count = (int)state->alerts.size();

    for (int i = 0; i < count; i++) {
        const auto& a = state->alerts[i];
        float ry = list_y + i * ALERT_ROW_H;
        if (ry + ALERT_ROW_H > max_y) break;

        // Row background
        SDL_Color row_bg = a.enabled ? tc.bg_row_even : tc.bg_row_odd;
        SDL_Color text_prim = tc.text_primary;
        SDL_Color text_sec = tc.text_secondary;
        SDL_Color text_mut = tc.text_muted;

        if (a.triggered && a.enabled) {
            row_bg    = {255, 200,  40, 255}; // Bright amber — high contrast
            text_prim = { 20,  20,  20, 255}; // Near-black
            text_sec  = { 40,  40,  40, 255};
            text_mut  = { 80,  60,   0, 255};
        }
        ui_fill_rect(r, x, ry, w, ALERT_ROW_H - 2.f, row_bg);
        ui_draw_rect(r, x, ry, w, ALERT_ROW_H - 2.f, tc.border);

        // Symbol
        std::string sym_line = "[" + a.symbol + "] " + alert_type_label(a.type);
        ui_draw_text(r, state->font_sm, x + 4.f, ry + 4.f, sym_line.c_str(), text_prim);

        // Threshold + note
        std::string val_line = "Threshold: " + fmt_price(a.threshold);
        if (!a.note.empty()) val_line += "  | " + a.note;
        ui_draw_text(r, state->font_sm, x + 4.f, ry + 18.f, val_line.c_str(), text_sec);

        // Freq + Telegram indicator
        std::string meta = alert_freq_label(a.frequency);
        if (a.via_telegram) meta += " | TG";
        ui_draw_text(r, state->font_sm, x + 4.f, ry + 32.f, meta.c_str(), text_mut);

        // Toggle button (ON/OFF)
        float tog_x = x + 4.f;
        float tog_y = ry + ALERT_ROW_H - 22.f;
        SDL_Color tog_bg = a.enabled ? tc.bull : tc.bg_btn_idle;
        ui_fill_rect(r, tog_x, tog_y, 38.f, 18.f, tog_bg);
        ui_draw_rect(r, tog_x, tog_y, 38.f, 18.f, tc.border);
        ui_draw_text_centered(r, state->font_sm, tog_x, tog_y, 38.f, 18.f,
                              a.enabled ? "ON" : "OFF", tc.text_primary);

        // Delete button
        ui_fill_rect(r, x + w - 24.f, ry + 4.f, 20.f, 18.f, tc.bear);
        ui_draw_text_centered(r, state->font_sm, x + w - 24.f, ry + 4.f, 20.f, 18.f,
                              "X", tc.text_primary);
    }
}

// =============================================================================
//  Alert create/edit modal
// =============================================================================

// Input field indices for the modal
enum class AlertField {
    SYMBOL = 0, THRESHOLD, NOTE, TG_BOT, TG_CHAT, COUNT
};

static int modal_active_field = 0; // which text field is focused

void draw_alert_modal(AppState* state) {
    if (!state->alert_modal_open) return;

    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    // Dim overlay
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_FRect full = {0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT};
    SDL_RenderFillRect(r, &full);

    // Modal box
    float mw = 480.f, mh = 420.f;
    float mx = ((float)WINDOW_WIDTH  - mw) * 0.5f;
    float my = ((float)WINDOW_HEIGHT - mh) * 0.5f;

    ui_fill_rect(r, mx, my, mw, mh, tc.bg_panel);
    ui_draw_rect(r, mx, my, mw, mh, tc.border_accent);

    // Title
    ui_fill_rect(r, mx, my, mw, 32.f, tc.bg_header);
    const char* title = state->alert_modal_is_new ? "New Alert" : "Edit Alert";
    ui_draw_text_centered(r, state->font_lg, mx, my, mw, 32.f, title, tc.text_header);

    Alert& a = state->alert_modal_draft;
    float fy = my + 38.f;
    float fw = mw - 20.f;
    float fh = 24.f;
    float lh = 18.f;

    auto field_label = [&](const char* lbl) {
        ui_draw_text(r, state->font_sm, mx + 10.f, fy, lbl, tc.text_muted);
        fy += lh;
    };
    auto field_input = [&](const std::string& val, AlertField fid) {
        bool focused = (modal_active_field == (int)fid);
        SDL_Color bg = focused ? tc.bg_input_focused : tc.bg_input;
        SDL_Color brd = focused ? tc.border_accent : tc.border;
        ui_fill_rect(r, mx + 10.f, fy, fw, fh, bg);
        ui_draw_rect(r, mx + 10.f, fy, fw, fh, brd);
        std::string disp = val + (focused ? "|" : "");
        if (disp.empty() && !focused) disp = "(click to edit)";
        ui_draw_text(r, state->font_sm, mx + 14.f, fy + 4.f, disp.c_str(), tc.text_input);
        fy += fh + 4.f;
    };

    // Symbol
    field_label("Symbol (e.g. BTCUSDT)");
    field_input(a.symbol, AlertField::SYMBOL);

    // Alert type (radio buttons, horizontal)
    ui_draw_text(r, state->font_sm, mx + 10.f, fy, "Alert Type", tc.text_muted);
    fy += lh;
    const AlertType types[] = {
        AlertType::PRICE_ABOVE, AlertType::PRICE_BELOW, AlertType::PRICE_REACHES,
        AlertType::CHANGE_24H_ABOVE, AlertType::CHANGE_24H_BELOW,
        AlertType::VOLUME_SPIKE, AlertType::VOLATILITY
    };
    const char* short_labels[] = {"P>", "P<", "P=", "C%>", "C%<", "Vol", "Vola"};
    float rbx = mx + 10.f;
    for (int i = 0; i < 7; i++) {
        bool sel = (a.type == types[i]);
        float rw2 = 56.f;
        SDL_Color bg = sel ? tc.bg_btn_active : tc.bg_btn_idle;
        ui_fill_rect(r, rbx, fy, rw2, 22.f, bg);
        ui_draw_rect(r, rbx, fy, rw2, 22.f, tc.border);
        ui_draw_text_centered(r, state->font_sm, rbx, fy, rw2, 22.f,
                              short_labels[i], tc.text_primary);
        rbx += rw2 + 2.f;
    }
    fy += 28.f;

    // Threshold
    field_label("Threshold value");
    std::string thr_str = fmt_price(a.threshold);
    field_input(thr_str, AlertField::THRESHOLD);

    // Note
    field_label("Note / Label (optional)");
    field_input(a.note, AlertField::NOTE);

    // Frequency row
    ui_draw_text(r, state->font_sm, mx + 10.f, fy, "Frequency", tc.text_muted);
    fy += lh;
    const AlertFrequency freqs[] = {AlertFrequency::ONCE, AlertFrequency::ALWAYS};
    const char* freq_names[]     = {"Once", "Always"};
    float fbx = mx + 10.f;
    for (int i = 0; i < 2; i++) {
        bool sel = (a.frequency == freqs[i]);
        float fw2 = 72.f;
        SDL_Color bg = sel ? tc.bg_btn_active : tc.bg_btn_idle;
        ui_fill_rect(r, fbx, fy, fw2, 22.f, bg);
        ui_draw_rect(r, fbx, fy, fw2, 22.f, tc.border);
        ui_draw_text_centered(r, state->font_sm, fbx, fy, fw2, 22.f,
                              freq_names[i], tc.text_primary);
        fbx += fw2 + 4.f;
    }
    // Telegram toggle
    bool tg_on = a.via_telegram;
    SDL_Color tg_bg = tg_on ? tc.bg_btn_active : tc.bg_btn_idle;
    ui_fill_rect(r, fbx, fy, 90.f, 22.f, tg_bg);
    ui_draw_rect(r, fbx, fy, 90.f, 22.f, tc.border);
    ui_draw_text_centered(r, state->font_sm, fbx, fy, 90.f, 22.f,
                          tg_on ? "📱 TG: ON" : "📱 TG: OFF", tc.text_primary);
    fy += 28.f;

    // Telegram fields (only if enabled)
    if (a.via_telegram) {
        field_label("Telegram Bot Token");
        field_input(a.tg_bot_token, AlertField::TG_BOT);
        field_label("Telegram Chat ID");
        field_input(a.tg_chat_id, AlertField::TG_CHAT);
    }

    // Action buttons
    float btn_w = 100.f, btn_h = 28.f;
    float btn_y2 = my + mh - btn_h - 10.f;

    // Cancel
    ui_fill_rect(r, mx + 10.f, btn_y2, btn_w, btn_h, tc.bg_btn_idle);
    ui_draw_rect(r, mx + 10.f, btn_y2, btn_w, btn_h, tc.border);
    ui_draw_text_centered(r, state->font, mx + 10.f, btn_y2, btn_w, btn_h,
                          "Cancel", tc.text_secondary);

    // Save
    ui_fill_rect(r, mx + mw - btn_w - 10.f, btn_y2, btn_w, btn_h, tc.bg_btn_active);
    ui_draw_rect(r, mx + mw - btn_w - 10.f, btn_y2, btn_w, btn_h, tc.border_accent);
    ui_draw_text_centered(r, state->font, mx + mw - btn_w - 10.f, btn_y2, btn_w, btn_h,
                          "Save Alert", tc.text_primary);
}

// ── Interactivity Helpers ────────────────────────────────────────────────────

static void get_modal_layout(float& mx, float& my, float& mw, float& mh) {
    mw = 480.f; mh = 420.f;
    mx = ((float)WINDOW_WIDTH  - mw) * 0.5f;
    my = ((float)WINDOW_HEIGHT - mh) * 0.5f;
}

bool handle_alert_modal_click(AppState* state, float mouse_x, float mouse_y) {
    if (!state->alert_modal_open) return false;
    
    float mx, my, mw, mh;
    get_modal_layout(mx, my, mw, mh);
    Alert& a = state->alert_modal_draft;
    
    // Type radios: fy = my + 38 + 18 + 24 + 4 + 18 (label) = my + 102
    float fy = my + 38.f + 18.f + 24.f + 4.f + 18.f;
    const AlertType types[] = {
        AlertType::PRICE_ABOVE, AlertType::PRICE_BELOW, AlertType::PRICE_REACHES,
        AlertType::CHANGE_24H_ABOVE, AlertType::CHANGE_24H_BELOW,
        AlertType::VOLUME_SPIKE, AlertType::VOLATILITY
    };
    float rbx = mx + 10.f;
    for (int i = 0; i < 7; i++) {
        float rw2 = 56.f;
        if (point_in(mouse_x, mouse_y, rbx, fy, rw2, 22.f)) {
            a.type = types[i];
            return true;
        }
        rbx += rw2 + 2.f;
    }
    fy += 28.f;
    
    // Skip Threshold and Note (input fields handled via text input for now, we just need radio logic)
    fy += 18.f + 24.f + 4.f; // Threshold
    fy += 18.f + 24.f + 4.f; // Note
    
    // Frequency row
    fy += 18.f; // label
    const AlertFrequency freqs[] = {AlertFrequency::ONCE, AlertFrequency::ALWAYS};
    float fbx = mx + 10.f;
    for (int i = 0; i < 2; i++) {
        float fw2 = 72.f;
        if (point_in(mouse_x, mouse_y, fbx, fy, fw2, 22.f)) {
            a.frequency = freqs[i];
            return true;
        }
        fbx += fw2 + 4.f;
    }
    
    // Telegram toggle
    if (point_in(mouse_x, mouse_y, fbx, fy, 90.f, 22.f)) {
        a.via_telegram = !a.via_telegram;
        return true;
    }
    
    // Actions are handled in main.cpp
    return true; // swallow click
}

bool update_alert_modal_cursor(AppState* state, float mouse_x, float mouse_y) {
    if (!state->alert_modal_open) return false;
    
    float mx, my, mw, mh;
    get_modal_layout(mx, my, mw, mh);
    
    // Cancel / Save
    float btn_w = 100.f, btn_h = 28.f;
    float btn_y2 = my + mh - btn_h - 10.f;
    if (point_in(mouse_x, mouse_y, mx + 10.f, btn_y2, btn_w, btn_h) ||
        point_in(mouse_x, mouse_y, mx + mw - btn_w - 10.f, btn_y2, btn_w, btn_h)) {
        SDL_SetCursor(state->cursor_hand);
        return true;
    }
    
    // Type radios
    float fy = my + 38.f + 18.f + 24.f + 4.f + 18.f;
    float rbx = mx + 10.f;
    for (int i = 0; i < 7; i++) {
        if (point_in(mouse_x, mouse_y, rbx, fy, 56.f, 22.f)) {
            SDL_SetCursor(state->cursor_hand);
            return true;
        }
        rbx += 56.f + 2.f;
    }
    fy += 28.f + (18.f + 24.f + 4.f) * 2 + 18.f;
    
    // Freq radios
    float fbx = mx + 10.f;
    for (int i = 0; i < 2; i++) {
        if (point_in(mouse_x, mouse_y, fbx, fy, 72.f, 22.f)) {
            SDL_SetCursor(state->cursor_hand);
            return true;
        }
        fbx += 72.f + 4.f;
    }
    if (point_in(mouse_x, mouse_y, fbx, fy, 90.f, 22.f)) {
        SDL_SetCursor(state->cursor_hand);
        return true;
    }
    
    if (point_in(mouse_x, mouse_y, mx, my, mw, mh)) {
        SDL_SetCursor(state->cursor_arrow);
    } else {
        SDL_SetCursor(state->cursor_arrow);
    }
    return true;
}

