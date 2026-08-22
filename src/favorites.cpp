#include "favorites.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
using json=nlohmann::json;

bool fav_has(const FavoritesState& f,const std::string& s){return f.symbols.count(s)>0;}
void fav_toggle(FavoritesState& f,const std::string& s){
    if(fav_has(f,s))f.symbols.erase(s); else f.symbols.insert(s);
}
void fav_save(const FavoritesState& f,const std::string& path){
    json j=json::array();
    for(auto& s:f.symbols)j.push_back(s);
    std::ofstream o(path); if(o)o<<j.dump(2);
}
void fav_load(FavoritesState& f,const std::string& path){
    std::ifstream i(path); if(!i)return;
    try{ json j; i>>j; for(auto& s:j)f.symbols.insert(s.get<std::string>()); }
    catch(...){}
}
