#include "file_cache.hpp"

#include "extension/fmt.hpp"
#include "general/logger.hpp"
#include "game/game_file.hpp"

namespace spellbook {

constexpr bool print_file_load_info = false;

json& FileCache::load_json(const string& file_path_input) {
    string file_path = file_path_input;
    std::replace(file_path.begin(), file_path.end(), '\\', '/');
    if (parsed_jsons.contains(file_path))
        return parsed_jsons[file_path];

    if (print_file_load_info)
        log(BasicMessage{.str=fmt_("Loading json: {}", file_path), .group = "asset"});
    parsed_jsons[file_path] = parse_file(file_path);
    load_dependencies(parsed_jsons[file_path]);
    return parsed_jsons[file_path];
}

AssetFile& FileCache::load_asset(const string& file_path_input) {
    string file_path = file_path_input;
    std::replace(file_path.begin(), file_path.end(), '\\', '/');
    if (parsed_assets.contains(file_path))
        return parsed_assets[file_path];

    if (print_file_load_info)
        log(BasicMessage{.str=fmt_("Loading asset: {}", file_path), .group = "asset"});
    return parsed_assets[file_path] = load_asset_file(file_path);
}

vector<string> FileCache::load_dependencies(json& j) {
    vector<string> list;
    if (j.contains("dependencies")) {
        for (const json_value& jv : j["dependencies"]->get_list()) {
            string& file_path = list.push_back(from_jv_impl(jv, (string*) 0));
            
            FileCategory category = file_category(file_type_from_path(fs::path(file_path)));
            if (category == FileCategory_Asset)
                this->load_asset(to_resource_path(file_path).string());
            if (category == FileCategory_Json)
                this->load_json(to_resource_path(file_path).string());
        }
    }
    return list;
}

}