#include "asset_loader.hpp"

#include <fstream>
#include <filesystem>

#include "general/logger.hpp"
#include "game/game_file.hpp"

namespace spellbook {

namespace fs = std::filesystem;


void save_asset_file(const AssetFile& asset_file) {
    std::ofstream outfile;
    outfile.open(to_resource_path(asset_file.file_name), std::ios::binary | std::ios::out);

    assert_else(outfile.is_open())
        return;

    string json_string = dump_json(asset_file.asset_json);

    const uint32 version     = asset_file.version;
    const uint32 json_length = (uint32) json_string.size();
    const uint32 blob_length = (uint32) asset_file.binary_blob.size();
    outfile.write(asset_file.type.data(), 4);
    outfile.write((const char*) &version, sizeof(uint32));
    outfile.write((const char*) &json_length, sizeof(uint32));
    outfile.write((const char*) &blob_length, sizeof(uint32));
    outfile.write(json_string.data(), json_length);
    outfile.write((const char*) asset_file.binary_blob.data(), asset_file.binary_blob.size());
    outfile.close();
}

AssetFile load_asset_file(const string& file_name) {
    std::filesystem::path file_path(to_resource_path(file_name));
    string                ext = file_path.extension().string();
    
    std::ifstream infile;
    infile.open(to_resource_path(file_name), std::ios::binary);

    assert_else(infile.is_open())
        return {};

    AssetFile asset_file;
    string    json_string;

    uint32 json_length = 0;
    uint32 blob_length = 0;
    infile.seekg(0);
    infile.read(asset_file.type.data(), 4);
    infile.read((char*) &asset_file.version, sizeof(uint32));
    infile.read((char*) &json_length, sizeof(uint32));
    infile.read((char*) &blob_length, sizeof(uint32));
    json_string.resize(json_length);
    asset_file.binary_blob.resize(blob_length);
    infile.read(json_string.data(), json_length);
    infile.read((char*) asset_file.binary_blob.data(), blob_length);

    asset_file.asset_json = parse(json_string);

    return asset_file;
}

}