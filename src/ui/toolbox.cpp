#include "ui/toolbox.hpp"

#include "core/constants.hpp"
#include "ui/ui_utils.hpp"

void draw_toolbox(AppState* state) {
  SDL_Renderer* r = state->renderer;
  const ThemeColors& tc = state->theme_colors;

  float tx = state->layout.toolbox_x;
  float ty = state->layout.toolbox_y;
  float tw = state->layout.toolbox_w;
  float th = state->layout.toolbox_h;

  ui_fill_rect(r, tx, ty, tw, th, tc.bg_panel);
  ui_draw_line(r, tx + tw - 1.f, ty, tx + tw - 1.f, ty + th, tc.border);

  using Tool = AppState::Tool;
  struct ToolBtn {
    const char* icon_glyph;
    const char* label;
    Tool t;
  };
  static const ToolBtn btns[] = {
    { ICON_CURSOR,     "Cursor",       Tool::CURSOR         },
    { ICON_CROSSHAIR,  "Crosshair",    Tool::CROSSHAIR      },
    { nullptr,         "Trend Line",   Tool::TREND_LINE     },
    { nullptr,         "Extended Tl.", Tool::TREND_LINE_EXT },
    { ICON_HORIZ_LINE, "Horiz Line",   Tool::HORIZ_LINE     },
    { ICON_TEXT_NOTE,  "Text Note",    Tool::TEXT_NOTE      },
  };
  constexpr int   NUM_TOOLS = 6;
  constexpr float ICON_H   = 34.f;
  constexpr float ICON_PAD = 2.f;

  int hovered_idx = -1;
  float hovered_by = 0;

  for (int i = 0; i < NUM_TOOLS; i++) {
    float by = ty + 6.f + i * (ICON_H + ICON_PAD);
    bool active = (state->active_tool == btns[i].t);
    bool hovered = (state->mouse_x >= tx && state->mouse_x < tx + tw &&
                    state->mouse_y >= by && state->mouse_y < by + ICON_H);

    if (hovered) { hovered_idx = i; hovered_by = by; }

    SDL_Color bg = active  ? tc.bg_btn_active
                 : hovered ? tc.bg_btn_idle
                 : tc.bg_panel;
    SDL_Color fg = active ? tc.text_primary : tc.text_muted;

    float bx = tx + 2.f;
    float bw = tw - 4.f;
    ui_fill_rect(r, bx, by, bw, ICON_H, bg);
    ui_draw_rect(r, bx, by, bw, ICON_H, active ? tc.border_accent : tc.border);

    if (btns[i].icon_glyph && state->font_icon_sm) {
      ui_draw_text_centered(r, state->font_icon_sm, bx, by, bw, ICON_H,
                            btns[i].icon_glyph, fg);
    } else {
      float cx = bx + bw * 0.5f;
      float cy = by + ICON_H * 0.5f;
      if (btns[i].t == Tool::TREND_LINE) {
        ui_draw_line(r, cx - 6, cy + 6, cx + 6, cy - 6, fg);
        ui_fill_rect(r, cx - 7, cy + 5, 3, 3, fg);
        ui_fill_rect(r, cx + 5, cy - 7, 3, 3, fg);
      } else if (btns[i].t == Tool::TREND_LINE_EXT) {
        ui_draw_line(r, cx - 8, cy + 8, cx + 8, cy - 8, fg);
        ui_fill_rect(r, cx - 9, cy + 7, 2, 2, fg);
        ui_fill_rect(r, cx + 7, cy - 9, 2, 2, fg);
        ui_draw_line(r, cx - 7, cy + 6, cx - 10, cy + 9, fg);
        ui_draw_line(r, cx + 7, cy - 8, cx + 10, cy - 11, fg);
      }
    }
  }

  constexpr float CLR_H = 28.f;
  float clr_y = ty + th - CLR_H - 6.f;
  float clr_bx = tx + 2.f;
  float clr_bw = tw - 4.f;
  bool clr_hovered = (state->mouse_x >= tx && state->mouse_x < tx + tw &&
                      state->mouse_y >= clr_y && state->mouse_y < clr_y + CLR_H);
  SDL_Color clr_bg = clr_hovered ? tc.bear : tc.bg_btn_idle;
  SDL_Color clr_fg = clr_hovered ? tc.text_primary : tc.text_muted;
  ui_fill_rect(r, clr_bx, clr_y, clr_bw, CLR_H, clr_bg);
  ui_draw_rect(r, clr_bx, clr_y, clr_bw, CLR_H, tc.border);
  float cc = clr_bx + clr_bw * 0.5f;
  float cm = clr_y + CLR_H * 0.5f;
  ui_draw_line(r, cc - 5, cm - 5, cc + 5, cm + 5, clr_fg);
  ui_draw_line(r, cc + 5, cm - 5, cc - 5, cm + 5, clr_fg);

  if (hovered_idx >= 0) {
    const char* label = btns[hovered_idx].label;
    int txt_w = ui_text_width(state->font_sm, label);
    float tt_x = tx + tw + 6.f;
    float tt_y = hovered_by + (ICON_H - 20.f) * 0.5f;
    ui_fill_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.bg_header);
    ui_draw_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.border);
    ui_draw_text(r, state->font_sm, tt_x + 6.f, tt_y + 3.f, label, tc.text_primary);
  } else if (clr_hovered) {
    int txt_w = ui_text_width(state->font_sm, "Clear All");
    float tt_x = tx + tw + 6.f;
    float tt_y = clr_y + (CLR_H - 20.f) * 0.5f;
    ui_fill_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.bg_header);
    ui_draw_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.border);
    ui_draw_text(r, state->font_sm, tt_x + 6.f, tt_y + 3.f, "Clear All", tc.text_primary);
  }
}

bool hit_toolbox_tool(AppState* state, float mx, float my, AppState::Tool& out_tool) {
  using Tool = AppState::Tool;
  static const Tool tool_order[] = {
    Tool::CURSOR, Tool::CROSSHAIR, Tool::TREND_LINE,
    Tool::TREND_LINE_EXT, Tool::HORIZ_LINE, Tool::TEXT_NOTE
  };
  constexpr int   NUM_TB   = 6;
  constexpr float ICON_H   = 34.f;
  constexpr float ICON_PAD = 2.f;
  float tbw = AppLayout::TOOLBOX_W;
  float ty_off = state->layout.toolbox_y;
  for (int i = 0; i < NUM_TB; i++) {
    float by = ty_off + 6.f + i * (ICON_H + ICON_PAD);
    if (point_in(mx, my, 2.f, by, tbw - 4.f, ICON_H)) {
      out_tool = tool_order[i];
      return true;
    }
  }
  return false;
}

bool hit_toolbox_clear(AppState* state, float mx, float my) {
  constexpr float CLR_H = 28.f;
  float tbw = AppLayout::TOOLBOX_W;
  float clr_y = state->layout.toolbox_y + state->layout.toolbox_h - CLR_H - 6.f;
  return point_in(mx, my, 2.f, clr_y, tbw - 4.f, CLR_H);
}
