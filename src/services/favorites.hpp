#pragma once
#include <string>
#include <unordered_set>

// =============================================================================
//  Trading Platform — Favorites
// =============================================================================

struct FavoritesState {
    std::unordered_set<std::string> symbols;
};

bool fav_has   (const FavoritesState& f, const std::string& sym);
void fav_toggle(FavoritesState& f, const std::string& sym);
void fav_save  (const FavoritesState& f, const std::string& path = "favorites.json");
void fav_load  (FavoritesState& f, const std::string& path = "favorites.json");
