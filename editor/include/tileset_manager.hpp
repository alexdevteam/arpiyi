#ifndef ARPIYI_TILESET_MANAGER_HPP
#define ARPIYI_TILESET_MANAGER_HPP

#include "asset_manager.hpp"
#include "util/math.hpp"
#include "assets/tileset.hpp"

#include <vector>

namespace arpiyi_editor::tileset_manager {

struct TilesetSelection {
    asset_manager::Handle<assets::Tileset> tileset;
    math::IVec2D selection_start;
    math::IVec2D selection_end;
};

void init();

void render();

std::size_t get_tile_size();

std::vector<asset_manager::Handle<assets::Tileset>>& get_tilesets();

TilesetSelection& get_selection();

}

#endif // ARPIYI_TILESET_MANAGER_HPP
