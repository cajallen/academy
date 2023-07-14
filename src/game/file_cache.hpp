#pragma once

#include "general/json.hpp"
#include "renderer/assets/asset_loader.hpp"

namespace spellbook {

struct FileCache {
    umap<string, json> parsed_jsons;
    umap<string, AssetFile> parsed_assets;

    json& load_json(const string& file_path);
    AssetFile& load_asset(const string& file_path);

    vector<string> load_dependencies(json& j);

    static FileCache& get() {
        static FileCache file_cache;
        return file_cache;
    }
};



}