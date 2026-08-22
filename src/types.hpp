#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <functional>

// =============================================================================
//  Trading Platform — Core Data Types
// =============================================================================

// ── Market data ──────────────────────────────────────────────────────────────

struct Candle {
    float   open, high, low, close, volume;
    int64_t open_time;
};

struct PairInfo {
    std::string symbol, base_asset, quote_asset;
    float price      = 0.f;
    float change_24h = 0.f;
    float volume_24h = 0.f;
    float high_24h   = 0.f;
    float low_24h    = 0.f;
    bool  is_futures = false;
};

struct OBEntry { float price, qty; };
struct OrderBook {
    std::vector<OBEntry> bids, asks;
    std::mutex           mtx;
};

struct Trade {
    int64_t id, time_ms;
    float   price, qty;
    bool    buyer_maker;
};

// ── Sorting / tabs ───────────────────────────────────────────────────────────

enum class SortColumn  { NAME, PRICE, CHANGE_24H, VOLUME_24H, HIGH_24H, LOW_24H };
enum class SortDir     { ASC, DESC };
enum class MarketTab   { SPOT, FUTURES, FAVORITES };
enum class RightTab    { ORDERBOOK, TRADES, ALERTS };
enum class Theme       { DARK, LIGHT, SYSTEM };

constexpr const char* TIMEFRAMES[]  = {"1m","3m","5m","15m","30m","1h","4h","1d","1w"};
constexpr int         NUM_TIMEFRAMES = 9;

// ── Alert system ─────────────────────────────────────────────────────────────

enum class AlertType {
    PRICE_ABOVE,        // price crosses above threshold
    PRICE_BELOW,        // price crosses below threshold
    PRICE_REACHES,      // price touches threshold (either direction)
    CHANGE_24H_ABOVE,   // 24 h % change > threshold
    CHANGE_24H_BELOW,   // 24 h % change < threshold (can be negative)
    VOLUME_SPIKE,       // volume_24h > threshold (absolute)
    VOLATILITY          // (high_24h - low_24h) / low_24h * 100 > threshold %
};

enum class AlertFrequency {
    ONCE,    // disable after first trigger
    ALWAYS   // re-arm each check interval
};

struct Alert {
    std::string    id;            // uuid-like string
    std::string    symbol;        // e.g. "BTCUSDT"
    AlertType      type        = AlertType::PRICE_ABOVE;
    AlertFrequency frequency   = AlertFrequency::ONCE;
    float          threshold   = 0.f;
    bool           enabled     = true;
    bool           triggered   = false; // last check triggered?
    bool           via_telegram= false;
    std::string    tg_bot_token;
    std::string    tg_chat_id;
    std::string    note;          // user label / description
};

// ── Search ───────────────────────────────────────────────────────────────────

struct SearchState {
    std::string query;            // current filter text
    bool        focused  = false;
    int         caret    = 0;     // cursor position inside query
};
