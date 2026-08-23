#include "app/app_event.hpp"

#include "core/app_state.hpp"
#include "core/constants.hpp"
#include "core/layout.hpp"
#include "core/types.hpp"
#include "modules/alerts.hpp"
#include "modules/market_table.hpp"
#include "modules/right_panel.hpp"
#include "services/favorites.hpp"
#include "ui/app_tabs.hpp"
#include "ui/theme.hpp"
#include "ui/toolbox.hpp"
#include "ui/ui_utils.hpp"
#include "window/titlebar.hpp"
#include "window/window_hit_test.hpp"
#include "window/window_snap.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

SDL_AppResult app_event(void* appstate, SDL_Event* event) {
  AppState* state = (AppState*)appstate;

  // update_layout needs window size — call before converting coordinates
  update_layout(state);

  // ── Window resize/move events ─────────────────────────────────────────────
  if (event->type == SDL_EVENT_WINDOW_RESIZED ||
      event->type == SDL_EVENT_WINDOW_MOVED) {
    update_layout(state);
    if (state->snap_drag_active)
      state->snap_zone = compute_snap_zone(state);
    return SDL_APP_CONTINUE;
  }

  // Convert to render (logical) coordinates for all UI hit-tests
  SDL_ConvertEventToRenderCoordinates(state->renderer, event);

  if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

  // ── Keyboard ──────────────────────────────────────────────────────────────
  if (event->type == SDL_EVENT_KEY_DOWN) {
    // Modal input handling
    if (state->alert_modal_open) {
      if (event->key.key == SDLK_ESCAPE) {
        state->alert_modal_open = false;
        SDL_StopTextInput(state->window);
      } else if (event->key.key == SDLK_BACKSPACE) {
        handle_alert_modal_backspace(state);
      }
      return SDL_APP_CONTINUE;
    }

    if (event->key.key == SDLK_ESCAPE) return SDL_APP_SUCCESS;

    // Ctrl+F → focus search
    if (event->key.key == SDLK_F &&
        (SDL_GetModState() & SDL_KMOD_CTRL)) {
      state->search.focused = true;
      return SDL_APP_CONTINUE;
    }

    // Search bar key input
    if (state->search.focused) {
      if (event->key.key == SDLK_BACKSPACE && !state->search.query.empty())
        state->search.query.pop_back();
      if (event->key.key == SDLK_ESCAPE || event->key.key == SDLK_RETURN)
        state->search.focused = false;
      return SDL_APP_CONTINUE;
    }
  }

  if (event->type == SDL_EVENT_TEXT_INPUT) {
    if (state->alert_modal_open) {
      handle_alert_modal_text(state, event->text.text);
      return SDL_APP_CONTINUE;
    }
    
    if (state->search.focused) {
      state->search.query += event->text.text;
      state->table_scroll = 0;
    }
    return SDL_APP_CONTINUE;
  }


  // ── Mouse wheel ───────────────────────────────────────────────────────────
  if (event->type == SDL_EVENT_MOUSE_WHEEL) {
    if (state->alert_modal_open) return SDL_APP_CONTINUE;

    float mx = event->wheel.mouse_x, my = event->wheel.mouse_y;

    if (mx < state->layout.chart_w && my >= state->layout.content_y) {
      // Chart side — zoom horizontally, or vertically over price axis
      float chart_x = state->layout.chart_x + AppLayout::TOOLBOX_W + 4.f;
      float chart_w = state->layout.chart_w - AppLayout::TOOLBOX_W - 14.f;
      float chart_area_x = chart_x + 10.f;
      float chart_area_w = chart_w - 95.f;
      if (mx > chart_area_x + chart_area_w) {
        // Over price axis: zoom vertically
        float factor = (event->wheel.y > 0) ? 1.1f : 0.9f;
        state->price_zoom *= factor;
        state->auto_scale = false;
      } else {
        // Over chart: zoom horizontally
        float factor = (event->wheel.y > 0) ? 1.18f : 0.85f;
        state->zoom_level *= factor;
        state->zoom_level  = std::max(0.1f, std::min(state->zoom_level, 20.f));
      }
    } else if (my >= state->layout.content_y) {
      // Right side — route to right-panel content or market table
      float rpx, rpy, rpw, rph;
      right_panel_content_rect(state, rpx, rpy, rpw, rph);
      if (point_in(mx, my, rpx, rpy, rpw, rph)) {
        switch (state->right_tab) {
          case RightTab::ORDERBOOK:
            state->ob_scroll -= (int)event->wheel.y * 2;
            state->ob_scroll  = std::max(0, state->ob_scroll);
            break;
          case RightTab::TRADES:
            state->trades_scroll -= (int)event->wheel.y * 2;
            state->trades_scroll  = std::max(0, state->trades_scroll);
            break;
          default: break;
        }
      } else {
        // Scrolling in market table area
        state->table_scroll -= (int)event->wheel.y * 3;
        state->table_scroll  = std::max(0, state->table_scroll);
      }
    }
  }

  // ── Mouse button DOWN ─────────────────────────────────────────────────────
  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    float mx = (float)event->button.x;
    float my = (float)event->button.y;

    if (event->button.button == SDL_BUTTON_LEFT) {
      // ── Titlebar interactions ──────────────────────────────────────────────
      if (my < AppLayout::TITLEBAR_H) {
        // Close
        if (hit_close_btn(state, mx, my)) return SDL_APP_SUCCESS;
        // Minimize
        if (hit_min_btn(state, mx, my)) {
          SDL_MinimizeWindow(state->window);
          return SDL_APP_CONTINUE;
        }
        // Maximize / Restore
        if (hit_max_btn(state, mx, my)) {
          toggle_window_maximize(state);
          return SDL_APP_CONTINUE;
        }
        if (event->button.clicks >= 2) {
          toggle_window_maximize(state);
          return SDL_APP_CONTINUE;
        }
        // Titlebar drag → track snap zones (native move via hit-test)
        begin_snap_drag(state, mx, my);
        return SDL_APP_CONTINUE;
      }

      // ── App tabs (below titlebar) ──────────────────────────────────────────
      int atab = hit_app_tab(state, mx, my);
      if (atab >= 0) {
        state->active_app_tab = (AppTab) atab;
        return SDL_APP_CONTINUE;
      }

      // ── Panel divider resize ───────────────────────────────────────────────
      if (state->active_app_tab == AppTab::MARKETS) {
        constexpr float HIT = 6.f;
        // Vertical divider: chart | right panel
        if (std::abs(mx - state->layout.chart_w) < HIT &&
            my >= state->layout.content_y) {
          state->h_resize_dragging = true;
          state->resize_drag_start = mx;
          state->resize_drag_ratio = state->layout.split_ratio;
          return SDL_APP_CONTINUE;
        }
        // Horizontal divider: table | right-tabs
        if (mx >= state->layout.table_x &&
            mx <= state->layout.table_x + state->layout.table_w &&
            std::abs(my - (state->layout.table_y + state->layout.table_h)) < HIT) {
          state->v_resize_dragging = true;
          state->resize_drag_start = my;
          state->resize_drag_ratio = state->layout.vsplit_ratio;
          return SDL_APP_CONTINUE;
        }
      }
    } // end LEFT button

    // ── Alert modal click handling ────────────────────────────────────────
    if (state->alert_modal_open) {
      float mw = 480.f, mh = 430.f; // Use updated size matching alerts.cpp
      float modal_x = ((float)state->layout.win_w  - mw) * 0.5f;
      float modal_y = ((float)state->layout.win_h - mh) * 0.5f;
      float btn_w = 100.f, btn_h = 28.f;
      float btn_y = modal_y + mh - btn_h - 10.f;

      // Cancel
      if (point_in(mx, my, modal_x + 10.f, btn_y, btn_w, btn_h)) {
        state->alert_modal_open = false;
        SDL_StopTextInput(state->window);
        return SDL_APP_CONTINUE;
      }
      // Save
      if (point_in(mx, my, modal_x + mw - btn_w - 10.f, btn_y, btn_w, btn_h)) {
        alert_commit_threshold(state);
        std::lock_guard<std::mutex> lock(state->alerts_mtx);
        if (state->alert_modal_is_new) {
          state->alert_modal_draft.id = alerts_new_id();
          state->alerts.push_back(state->alert_modal_draft);
        } else {
          for (auto& a : state->alerts)
            if (a.id == state->alert_modal_draft.id)
              a = state->alert_modal_draft;
        }
        alerts_save(state->alerts, ALERTS_FILE);
        state->alert_modal_open = false;
        SDL_StopTextInput(state->window);
        return SDL_APP_CONTINUE;
      }

      // Configuration / Inputs
      handle_alert_modal_click(state, mx, my);
      return SDL_APP_CONTINUE; // swallow all other modal clicks
    }


    // ── Markets tab content interactions ─────────────────────────────────────
    if (state->active_app_tab != AppTab::MARKETS) return SDL_APP_CONTINUE;
    // Only process content-area clicks here
    float mx2 = (float)event->button.x;
    float my2 = (float)event->button.y;

    // ── Chart area clicks ─────────────────────────────────────────────────
    if (mx2 < state->layout.chart_w && my2 >= state->layout.content_y) {
      // ── Toolbox buttons (leftmost strip) ─────────────────────────────
      if (mx2 < AppLayout::TOOLBOX_W) {
        AppState::Tool picked;
        if (hit_toolbox_tool(state, mx2, my2, picked)) {
          state->active_tool = picked;
          return SDL_APP_CONTINUE;
        }
        if (hit_toolbox_clear(state, mx2, my2)) {
          state->drawings.clear();
          state->drawing_in_progress = false;
          return SDL_APP_CONTINUE;
        }
        return SDL_APP_CONTINUE;
      }

      // Double-click on price axis → reset auto-scale
      if (event->button.clicks >= 2) {
        float tbw2       = AppLayout::TOOLBOX_W;
        float cx_off     = state->layout.chart_x + tbw2 + 4.f;
        float cw_off     = state->layout.chart_w  - tbw2 - 14.f;
        float chart_ax   = cx_off + 10.f;
        float chart_aw   = cw_off - 95.f;
        if (mx2 > chart_ax + chart_aw) {
          state->auto_scale   = true;
          state->price_zoom   = 1.0f;
          state->price_scroll = 0.0f;
        }
      }

      // Chart panel local coordinates (offset by toolbox)
      float tbw2   = AppLayout::TOOLBOX_W;
      float cx_off = state->layout.chart_x + tbw2 + 4.f;
      float cy_off2 = state->layout.chart_y + 10.f;
      float cw_off = state->layout.chart_w - tbw2 - 14.f;

      // Timeframe buttons
      float btn_x_start = cx_off + cw_off - (float)(NUM_TIMEFRAMES * 54 + 10);
      float btn_y_v     = cy_off2 + 6.f;
      for (int i = 0; i < NUM_TIMEFRAMES; i++) {
        float bx = btn_x_start + i * 54.f;
        if (point_in(mx2, my2, bx, btn_y_v, 50.f, 24.f)) {
          state->tf_index    = i;
          state->view_offset = 0;
          std::lock_guard<std::mutex> lk(state->candles_mtx);
          state->candles.clear();
          return SDL_APP_CONTINUE;
        }
      }

      // Chart drawing area hit-test
      float chart_area_x = cx_off + 10.f;
      float chart_area_y = cy_off2 + 46.f;
      float chart_area_w = cw_off - 95.f;
      float chart_area_h = (state->layout.chart_h - 20.f) - 80.f;
      if (point_in(mx2, my2, chart_area_x, chart_area_y, chart_area_w, chart_area_h)) {
        if (state->active_tool != AppState::Tool::CURSOR && state->active_tool != AppState::Tool::CROSSHAIR) {
          state->drawing_in_progress = true;
          state->current_drawing.tool = state->active_tool;
          state->current_drawing.t1 = state->hover_time;
          state->current_drawing.p1 = state->hover_price;
          state->current_drawing.t2 = state->hover_time;
          state->current_drawing.p2 = state->hover_price;
          if (state->active_tool == AppState::Tool::TEXT_NOTE) {
            state->current_drawing.text = "Note";
          }
        } else {
          state->chart_dragging = true;
          state->drag_start_mx  = mx2;
          state->drag_start_my  = my2;
          state->drag_start_offset = state->view_offset;
          state->drag_start_price_scroll = state->price_scroll;
          state->auto_scale = false;
        }
      } else if (point_in(mx2, my2, chart_area_x + chart_area_w, chart_area_y, 95.f, chart_area_h)) {
        state->y_axis_dragging = true;
        state->drag_start_my   = my2;
        state->drag_start_price_zoom = state->price_zoom;
        state->auto_scale = false;
      }
      return SDL_APP_CONTINUE;
    }

    // ── Right panel area clicks ────────────────────────────────────────────────
    if (mx2 >= state->layout.table_x && my2 >= state->layout.content_y) {
      // Right-panel tabs: ORDER BOOK | TRADES | ALERTS
      int rtab = hit_right_tab(state, mx2, my2);
      if (rtab >= 0) {
        state->right_tab     = (RightTab)rtab;
        state->ob_scroll     = 0;
        state->trades_scroll = 0;
        return SDL_APP_CONTINUE;
      }

      // Alerts add / row actions
      if (state->right_tab == RightTab::ALERTS) {
        float rpx, rpy, rpw, rph;
        right_panel_content_rect(state, rpx, rpy, rpw, rph);
        if (hit_alerts_add_button(mx2, my2, rpx, rpy, rpw)) {
          state->alert_modal_is_new  = true;
          state->alert_modal_draft   = Alert{};
          state->alert_modal_draft.symbol = state->chart_symbol;
          state->alert_threshold_buf = "0.0";
          state->alert_modal_field   = -1;
          state->alert_modal_open    = true;
          return SDL_APP_CONTINUE;
        }
        std::lock_guard<std::mutex> lock(state->alerts_mtx);
        int count = (int)state->alerts.size();
        int ridx = hit_alert_row(mx2, my2, rpx, rpy, rpw, rph, count);
        if (ridx >= 0 && ridx < count) {
          float row_y = rpy + 22.f + 24.f + 4.f + ridx * 58.f;
          if (hit_alert_del_btn(mx2, my2, rpx, row_y, rpw)) {
            state->alerts.erase(state->alerts.begin() + ridx);
            alerts_save(state->alerts, ALERTS_FILE);
          } else if (hit_alert_toggle_btn(mx2, my2, rpx, row_y)) {
            state->alerts[ridx].enabled = !state->alerts[ridx].enabled;
            alerts_save(state->alerts, ALERTS_FILE);
          } else {
            // Edit existing alert
            state->alert_modal_is_new  = false;
            state->alert_modal_draft   = state->alerts[ridx];
            std::ostringstream ss;
            ss << state->alert_modal_draft.threshold;
            state->alert_threshold_buf = ss.str();
            state->alert_modal_field   = -1;
            state->alert_modal_open    = true;
          }
        }
        return SDL_APP_CONTINUE;
      }


      // Theme toggle
      if (hit_theme_button(state, mx2, my2)) {
        state->theme        = theme_next(state->theme);
        state->theme_colors = get_theme(state->theme);
        return SDL_APP_CONTINUE;
      }

      // Search bar focus
      if (hit_search_bar(state, mx2, my2)) {
        state->search.focused = true;
        SDL_StartTextInput(state->window);
        return SDL_APP_CONTINUE;
      } else if (state->search.focused) {
        state->search.focused = false;
        SDL_StopTextInput(state->window);
      }

      // Market tabs (SPOT / FUTURES / FAV)
      int tab = hit_tab(state, mx2, my2);
      if (tab >= 0) {
        state->active_tab   = (MarketTab)tab;
        state->table_scroll = 0;
        return SDL_APP_CONTINUE;
      }

      // Column header sort
      int col = hit_col_header(state, mx2, my2);
      if (col >= 0) {
        SortColumn sc_map[] = {
          SortColumn::NAME, SortColumn::PRICE, SortColumn::CHANGE_24H,
          SortColumn::VOLUME_24H, SortColumn::HIGH_24H, SortColumn::LOW_24H
        };
        SortColumn new_sc = sc_map[col];
        if (state->sort_col == new_sc)
          state->sort_dir = (state->sort_dir == SortDir::ASC) ? SortDir::DESC : SortDir::ASC;
        else { state->sort_col = new_sc; state->sort_dir = SortDir::DESC; }
        state->table_scroll = 0;
        return SDL_APP_CONTINUE;
      }

      // Row click — select symbol or toggle favourite
      auto sorted = get_sorted_filtered_pairs(state);
      int row = hit_table_row(mx2, my2, state, sorted);
      if (row >= 0 && row < (int)sorted.size()) {
        int row_idx = row - state->table_scroll;
        if (hit_fav_star(state, mx2, my2, row_idx)) {
          fav_toggle(state->favorites, sorted[row].symbol);
          fav_save(state->favorites, FAVORITES_FILE);
        } else {
          std::string new_sym = sorted[row].symbol;
          if (new_sym != state->chart_symbol) {
            state->chart_symbol = new_sym;
            state->view_offset  = 0;
            std::lock_guard<std::mutex> lk(state->candles_mtx);
            state->candles.clear();
          }
        }
      }
    } // end right side
  }

  // ── Mouse button UP ───────────────────────────────────────────────────────
  if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
    if (event->button.button == SDL_BUTTON_LEFT) {
      if (state->snap_drag_active) {
        if (state->snap_zone != SnapZone::NONE)
          apply_snap(state, state->snap_zone);
        state->snap_drag_active = false;
        state->snap_zone = SnapZone::NONE;
      }
      if (state->drawing_in_progress) {
        state->drawings.push_back(state->current_drawing);
        state->drawing_in_progress = false;
      }
      state->chart_dragging      = false;
      state->y_axis_dragging     = false;
      state->h_resize_dragging   = false;
      state->v_resize_dragging   = false;
      state->titlebar_dragging   = false;
      state->border_resize_drag  = false;
      state->resize_dir          = ResizeDir::NONE;
    }
  }

  // ── Mouse motion ─────────────────────────────────────────────────────────
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    float mx = (float)event->motion.x;
    float my = (float)event->motion.y;
    state->mouse_x = mx;
    state->mouse_y = my;

    // ── Modal Cursor handling ─────────────────────────────────────────────
    if (state->alert_modal_open) {
      update_alert_modal_cursor(state, mx, my);
      return SDL_APP_CONTINUE;
    }

    // ── Panel resize drags ────────────────────────────────────────────────
    if (state->h_resize_dragging) {
      float delta = mx - state->resize_drag_start;
      state->layout.split_ratio = std::max(0.40f, std::min(0.85f,
        state->resize_drag_ratio + delta / state->layout.win_w));
      SDL_SetCursor(state->cursor_resize_ew);
      return SDL_APP_CONTINUE;
    }
    if (state->v_resize_dragging) {
      float delta = my - state->resize_drag_start;
      state->layout.vsplit_ratio = std::max(0.20f, std::min(0.80f,
        state->resize_drag_ratio + delta / state->layout.content_h));
      SDL_SetCursor(state->cursor_resize_ns);
      return SDL_APP_CONTINUE;
    }

    // ── Chart drags & drawing ─────────────────────────────────────────────
    if (state->drawing_in_progress) {
      state->current_drawing.t2 = state->hover_time;
      state->current_drawing.p2 = state->hover_price;
    } else if (state->chart_dragging) {
      float dx = mx - state->drag_start_mx;
      float dy = my - state->drag_start_my;
      int visible = (int)(80.f * state->zoom_level);
      float tbw     = AppLayout::TOOLBOX_W;
      float chart_w = state->layout.chart_w - tbw - 14.f - 95.f;
      float candle_w = chart_w / (float)visible;
      int offset_shift = (int)(dx / candle_w);
      state->view_offset = std::max(0, state->drag_start_offset + offset_shift);
      float chart_h = (state->layout.chart_h - 20.f) - 80.f;
      state->price_scroll = state->drag_start_price_scroll + (dy / chart_h);
    } else if (state->y_axis_dragging) {
      float dy = my - state->drag_start_my;
      float factor = std::exp(-dy * 0.01f);
      state->price_zoom = state->drag_start_price_zoom * factor;
      state->price_zoom = std::max(0.01f, std::min(state->price_zoom, 100.f));
    }

    // ── Cursor shape based on position ────────────────────────────────────
    // Border resize zones (highest priority)
    ResizeDir rd = get_resize_dir(state, mx, my);
    if (rd != ResizeDir::NONE) {
      SDL_SetCursor(cursor_for_resize(state, rd));
      return SDL_APP_CONTINUE;
    }

    // Titlebar
    if (my < AppLayout::TITLEBAR_H) {
      SDL_SetCursor(state->cursor_arrow);
      return SDL_APP_CONTINUE;
    }
    // App tabs
    if (my < AppLayout::TITLEBAR_H + AppLayout::APPTAB_H) {
      SDL_SetCursor(state->cursor_hand);
      return SDL_APP_CONTINUE;
    }

    constexpr float HIT = 6.f;
    float tbw    = AppLayout::TOOLBOX_W;
    float cx_off = state->layout.chart_x + tbw + 4.f;
    float cy_off = state->layout.chart_y + 10.f;
    float cw_off = state->layout.chart_w  - tbw - 14.f;
    float chart_ax   = cx_off + 10.f;
    float chart_ay   = cy_off + 46.f;
    float chart_aw   = cw_off - 95.f;
    float chart_ah   = (state->layout.chart_h - 20.f) - 80.f;

    if (std::abs(mx - state->layout.chart_w) < HIT && my >= state->layout.content_y) {
      SDL_SetCursor(state->cursor_resize_ew);
    } else if (mx >= state->layout.table_x &&
               mx <= state->layout.table_x + state->layout.table_w &&
               std::abs(my - (state->layout.table_y + state->layout.table_h)) < HIT) {
      SDL_SetCursor(state->cursor_resize_ns);
    } else if (mx >= state->layout.table_x && my >= state->layout.content_y) {
      auto sorted = get_sorted_filtered_pairs(state);
      state->table_hover = hit_table_row(mx, my, state, sorted);

      bool hand = hit_theme_button(state, mx, my) || hit_search_bar(state, mx, my) ||
                  hit_tab(state, mx, my) >= 0 || hit_col_header(state, mx, my) >= 0 ||
                  hit_right_tab(state, mx, my) >= 0 || state->table_hover >= 0;

      // Alerts panel interactive elements
      if (!hand && state->right_tab == RightTab::ALERTS) {
        float rpx, rpy, rpw, rph;
        right_panel_content_rect(state, rpx, rpy, rpw, rph);
        if (hit_alerts_add_button(mx, my, rpx, rpy, rpw)) {
          hand = true;
        } else {
          std::lock_guard<std::mutex> lock(state->alerts_mtx);
          int count = (int)state->alerts.size();
          int ridx = hit_alert_row(mx, my, rpx, rpy, rpw, rph, count);
          if (ridx >= 0 && ridx < count) {
            float row_y = rpy + 22.f + 24.f + 4.f + ridx * 58.f;
            if (hit_alert_del_btn(mx, my, rpx, row_y, rpw) ||
                hit_alert_toggle_btn(mx, my, rpx, row_y)) {
              hand = true;
            }
          }
        }
      }

      SDL_SetCursor(hand ? state->cursor_hand : state->cursor_arrow);
    } else if (mx < tbw && my >= state->layout.content_y) {
      // Toolbox strip
      SDL_SetCursor(state->cursor_hand);
    } else {
      state->table_hover = -1;

      if (state->chart_dragging || state->y_axis_dragging) {
        SDL_SetCursor(state->cursor_move);
      } else if (mx > chart_ax + chart_aw && my >= chart_ay && my <= chart_ay + chart_ah) {
        SDL_SetCursor(state->cursor_resize_ns);
      } else if (point_in(mx, my, chart_ax, chart_ay, chart_aw, chart_ah)) {
        SDL_SetCursor(state->cursor_cross);
      } else if (my > chart_ay + chart_ah && mx >= chart_ax && mx <= chart_ax + chart_aw) {
        SDL_SetCursor(state->cursor_resize_ew);
      } else {
        SDL_SetCursor(state->cursor_arrow);
      }
    }
  }

  return SDL_APP_CONTINUE;
}
