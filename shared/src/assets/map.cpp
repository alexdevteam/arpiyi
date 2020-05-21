#include "assets/map.hpp"
#include "global_tile_size.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "renderer/renderer.hpp"
#include "util/defs.hpp"

#include <vector>

namespace arpiyi::assets {

// TODO: Codegenize this
Sprite Map::Tile::sprite(Layer const& this_layer, math::IVec2D this_pos) const {
    if (!exists) {
        return Sprite{{Handle<assets::Texture>::noid}};
    }
    assert(parent.tileset.get());
    switch (parent.tileset.get()->tile_type) {
        case TileType::normal: return impl_sprite<TileType::normal>(this_layer, this_pos);
        case TileType::rpgmaker_a2: return impl_sprite<TileType::rpgmaker_a2>(this_layer, this_pos);
        default: assert(false && "Unknown tileset type"); return {};
    }
}

Map::TileConnections Map::Tile::calculate_connections(TileSurroundings const& surroundings) const {
    if (override_connections)
        return custom_connections;
    const auto compare_or_false = [this](const assets::Tileset::Tile* tile) -> bool {
        if (tile == nullptr)
            return false;
        return tile->tileset == parent.tileset && tile->tile_index == parent.tile_index;
    };
    /* clang-format off */
    return {
        compare_or_false(surroundings.down),
        compare_or_false(surroundings.down_right),
        compare_or_false(surroundings.right),
        compare_or_false(surroundings.up_right),
        compare_or_false(surroundings.up),
        compare_or_false(surroundings.up_left),
        compare_or_false(surroundings.left),
        compare_or_false(surroundings.down_left)
    };
    /* clang-format on */
}

[[nodiscard]] Map::TileSurroundings Map::Layer::get_surroundings(math::IVec2D pos) const {
    Map::TileSurroundings surroundings;
    const auto get_or_null = [this](math::IVec2D pos) -> Tileset::Tile const* {
        return is_pos_valid(pos) ? &get_tile(pos).parent : nullptr;
    };

    surroundings.down = get_or_null({pos.x, pos.y - 1});
    surroundings.down_right = get_or_null({pos.x + 1, pos.y - 1});
    surroundings.right = get_or_null({pos.x + 1, pos.y});
    surroundings.up_right = get_or_null({pos.x + 1, pos.y + 1});
    surroundings.up = get_or_null({pos.x, pos.y + 1});
    surroundings.up_left = get_or_null({pos.x - 1, pos.y + 1});
    surroundings.left = get_or_null({pos.x - 1, pos.y});
    surroundings.down_left = get_or_null({pos.x - 1, pos.y - 1});

    return surroundings;
}

Map::Layer::Layer(i64 width, i64 height) : width(width), height(height), tiles(width * height) {}

namespace map_file_definitions {

constexpr std::string_view name_json_key = "name";
constexpr std::string_view width_json_key = "width";
constexpr std::string_view height_json_key = "height";
constexpr std::string_view layers_json_key = "layers";
constexpr std::string_view comments_json_key = "comments";
constexpr std::string_view entities_json_key = "entities";

namespace layer_file_definitions {
constexpr std::string_view name_json_key = "name";
constexpr std::string_view data_json_key = "data";
} // namespace layer_file_definitions

namespace comment_file_definitions {
constexpr std::string_view position_json_key = "pos";
constexpr std::string_view text_json_key = "text";
} // namespace comment_file_definitions

} // namespace map_file_definitions

template<> RawSaveData raw_get_save_data<Map>(Map const& map) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);

    using namespace map_file_definitions;

    w.StartObject();
    w.Key(name_json_key.data());
    w.String(map.name.c_str());
    w.Key(width_json_key.data());
    w.Int64(map.width);
    w.Key(height_json_key.data());
    w.Int64(map.height);
    w.Key(layers_json_key.data());
    w.StartArray();
    for (const auto& _l : map.layers) {
        auto& layer = *_l.get();
        namespace lfd = layer_file_definitions;
        w.StartObject();
        w.Key(lfd::name_json_key.data());
        w.String(layer.name.data());
        w.Key(lfd::data_json_key.data());
        w.StartArray();
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                const auto& tile = layer.get_tile({x, y});
                w.StartObject();
                if (!tile.exists) {
                    w.Key("exists");
                    w.Bool(false);
                } else {
                    {
                        w.Key("parent");
                        w.StartObject();
                        w.Key("tileset");
                        w.Uint64(tile.parent.tileset.get_id());
                        w.Key("tile_index");
                        w.Uint64(tile.parent.tile_index);
                        w.EndObject();
                    }
                    if (tile.override_connections) {
                        w.Key("custom_connections");
                        w.StartArray();
                        w.Bool(tile.custom_connections.down);
                        w.Bool(tile.custom_connections.down_right);
                        w.Bool(tile.custom_connections.right);
                        w.Bool(tile.custom_connections.up_right);
                        w.Bool(tile.custom_connections.up);
                        w.Bool(tile.custom_connections.up_left);
                        w.Bool(tile.custom_connections.left);
                        w.Bool(tile.custom_connections.down_left);
                        w.EndArray();
                    }
                    {
                        w.Key("height");
                        w.Int(tile.height);
                    }
                    {
                        w.Key("slope_type");
                        w.Uint(static_cast<u8>(tile.slope_type));
                    }
                    {
                        w.Key("has_side_walls");
                        w.Bool(tile.has_side_walls);
                    }
                }
                w.EndObject();
            }
        }
        w.EndArray();
        w.EndObject();
    }
    w.EndArray();
    w.Key(comments_json_key.data());
    w.StartArray();
    for (const auto& c : map.comments) {
        namespace cfd = comment_file_definitions;
        assert(c.get());
        const auto& comment = *c.get();
        w.StartObject();
        w.Key(cfd::text_json_key.data());
        w.String(comment.text.c_str());
        w.Key(cfd::position_json_key.data());
        w.StartObject();
        {
            w.Key("x");
            w.Int(comment.pos.x);
            w.Key("y");
            w.Int(comment.pos.y);
        }
        w.EndObject();
        w.EndObject();
    }
    w.EndArray();

    w.Key(entities_json_key.data());
    w.StartArray();
    for (const auto& c : map.entities) { w.Uint64(c.get_id()); }
    w.EndArray();

    w.EndObject();

    RawSaveData data;
    data.bytestream.write(s.GetString(), s.GetLength());

    return data;
}

template<> void raw_load<Map>(Map& map, LoadParams<Map> const& params) {
    // TODO: This is entirely outdated, finish

    /*
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    map.width = -1;
    map.height = -1;

    using namespace map_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == name_json_key.data()) {
            map.name = obj.value.GetString();
        } else if (obj.name == width_json_key.data()) {
            map.width = obj.value.GetInt64();
        } else if (obj.name == height_json_key.data()) {
            map.height = obj.value.GetInt64();
        } else if (obj.name == layers_json_key.data()) {
            for (auto const& layer_object : obj.value.GetArray()) {
                namespace lfd = layer_file_definitions;
                if (map.width == -1 || map.height == -1) {
                    assert("Map layer data loaded before width/height");
                }
                auto& layer = *map.layers
                                   .emplace_back(asset_manager::put(
                                       assets::Map::Layer(map.width, map.height, -1)))
                                   .get();

                for (auto const& layer_val : layer_object.GetObject()) {
                    if (layer_val.name == lfd::name_json_key.data()) {
                        layer.name = layer_val.value.GetString();
                        // < 0.1.4 compatibility: Blank layer names are no longer allowed
                        if (layer.name == "") {
                            layer.name = "<Blank name>";
                        }
                    } else if (layer_val.name == lfd::tileset_id_json_key.data()) {
                        layer.tileset = Handle<Tileset>(layer_val.value.GetUint64());
                        layer.regenerate_mesh();
                    } else if (layer_val.name == lfd::data_json_key.data()) {
                        u64 i = 0;
                        for (auto const& layer_tile : layer_val.value.GetArray()) {
                            layer
                                .get_tile({static_cast<i32>(i % map.width),
                                           static_cast<i32>(i / map.width)})
                                .id = layer_tile.GetUint();
                            ++i;
                        }
                        layer.regenerate_mesh();
                    } else if (layer_val.name == lfd::depth_data_json_key.data()) {
                        u64 i = 0;
                        for (auto const& tile_depth : layer_val.value.GetArray()) {
                            layer
                                .get_tile({static_cast<i32>(i % map.width),
                                           static_cast<i32>(i / map.width)})
                                .height = tile_depth.GetInt();
                            ++i;
                        }
                        layer.regenerate_mesh();
                    }
                }
            }
        } else if (obj.name == comments_json_key.data()) {
            for (auto const& comment_object : obj.value.GetArray()) {
                namespace cfd = comment_file_definitions;

                assets::Map::Comment comment;

                for (auto const& comment_val : comment_object.GetObject()) {
                    if (comment_val.name == cfd::text_json_key.data()) {
                        comment.text = comment_val.value.GetString();
                    } else if (comment_val.name == cfd::position_json_key.data()) {
                        comment.pos.x = comment_val.value.GetObject()["x"].GetInt();
                        comment.pos.y = comment_val.value.GetObject()["y"].GetInt();
                    }
                }

                map.comments.emplace_back(asset_manager::put(comment));
            }
        } else if (obj.name == entities_json_key.data()) {
            for (const auto& entity_id : obj.value.GetArray()) {
                map.entities.emplace_back(entity_id.GetUint64());
            }
        }
    }
     */
}
} // namespace arpiyi::assets
