#pragma once

// =============================================================================
//  Trading Platform — Constants
// =============================================================================

#define WINDOW_WIDTH    1600
#define WINDOW_HEIGHT   900
#define CHART_WIDTH     900
#define TABLE_WIDTH     (WINDOW_WIDTH - CHART_WIDTH)

#define FONT_PATH       "res/fonts/JetBrainsMono-Regular.ttf"
#define ICON_FONT_PATH  "res/fonts/sdl-trading.ttf"

#define ALERTS_FILE     "alerts.json"
#define FAVORITES_FILE  "favorites.json"

// ── Icon codepoints (sdl-trading.ttf) ────────────────────────────────────────
// U+F000 □  window/maximize frame
// U+F001 T  text note tool
// U+F002 +  crosshair tool
// U+F003 ─●─ horizontal line tool
// U+F004 ▷  cursor/arrow tool
// U+F005 ✕  close / X
// U+F006 ⊟  restore window (two overlapping squares)
// U+F007 —  minimize (dash)

// UTF-8 encoded private-use codepoints
#define ICON_MAXIMIZE   "\xEF\x80\x80"   // U+F000
#define ICON_TEXT_NOTE  "\xEF\x80\x81"   // U+F001
#define ICON_CROSSHAIR  "\xEF\x80\x82"   // U+F002
#define ICON_HORIZ_LINE "\xEF\x80\x83"   // U+F003
#define ICON_CURSOR     "\xEF\x80\x84"   // U+F004
#define ICON_CLOSE      "\xEF\x80\x85"   // U+F005
#define ICON_RESTORE    "\xEF\x80\x86"   // U+F006
#define ICON_MINIMIZE   "\xEF\x80\x87"   // U+F007
