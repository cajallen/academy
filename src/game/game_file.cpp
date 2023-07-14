#include "game_file.hpp"

#include "extension/imgui_extra.hpp"
#include "general/file.hpp"
#include "editor/editor.hpp"
#include "renderer/assets/model.hpp"

namespace spellbook {

fs::path to_resource_path(const fs::path& path) {
    fs::path ret;
    if (path.is_relative()) {
        if (path.string().starts_with(fs::path(get_editor().resource_folder).lexically_proximate(fs::current_path()).string()))
            return (fs::current_path() / path).string();
        return (get_editor().resource_folder / path).string();
    }
    return path;
}

void create_resource_directory(const fs::path& output_folder) {
    fs::create_directory(get_editor().resource_folder);
    auto folder = get_editor().resource_folder / output_folder;
    fs::create_directory(folder);
}


FileType file_type_from_path(const fs::path& path) {
    string extension_string = path.extension().string();
    for (uint32 i = 0; i < magic_enum::enum_count<FileType>(); i++) {
        FileType type = (FileType) i;
        if (extension(type) == extension_string)
            return type;
    }
    return FileType_General;
}

string extension(FileType type) {
    switch (type) {
        case (FileType_General):
            return ".sbgen";
        case (FileType_Model):
            return ".sbmod";
        case (FileType_Texture):
            return ".sbtex";
        case (FileType_Mesh):
            return ".sbmsh";
        case (FileType_Material):
            return ".sbmat";
    }
    return "NYI";
}

std::function<bool(const fs::path&)> path_filter(FileType type) {
     switch (type) {
         case (FileType_Directory):
             return [](const fs::path& path) { return is_directory(path); };
         case (FileType_General):
             return [](const fs::path& path) { return is_regular_file(path); };
         case (FileType_ModelAsset):
             return [](const fs::path& path) { return vector<string>{".gltf", ".glb"}.contains(path.extension().string()); };
         case (FileType_TextureAsset):
             return [](const fs::path& path) { return vector<string>{".png", ".jpg", ".jpeg"}.contains(path.extension().string()); };
         default:
             return [type](const fs::path& path) { return path.extension().string() == extension(type); };
     }
}

string dnd_key(FileType type) {
    switch (type) {
        case (FileType_Directory):
            return "DND_DIRECTORY";
        case (FileType_General):
            return "DND_GENERAL";
        case (FileType_Model):
            return "DND_MODEL";
        case (FileType_ModelAsset):
            return "DND_MODEL_ASSET";
        case (FileType_Texture):
            return "DND_TEXTURE";
        case (FileType_TextureAsset):
            return "DND_TEXTURE_ASSET";
        case (FileType_Mesh):
            return "DND_MESH";
        case (FileType_Material):
            return "DND_MATERIAL";
    }
    return "DND_UNKNOWN";
}

FileType from_typeinfo(const type_info& input) {
    if (input == typeid(ModelCPU))
        return FileType_Model;
    if (input == typeid(TextureCPU))
        return FileType_Texture;
    if (input == typeid(MeshCPU))
        return FileType_Mesh;
    if (input == typeid(MaterialCPU))
        return FileType_Material;
    
    log_error("extension NYI");
    return FileType_General;
}

FileCategory file_category(FileType type) {
    switch (type) {
        case (FileType_Texture):
        case (FileType_Mesh):
            return FileCategory_Asset;
        case (FileType_General):
        case (FileType_Model):
        case (FileType_Material):
            return FileCategory_Json;
        default:
            return FileCategory_Other;
    }
}

bool inspect_dependencies(vector<string>& dependencies, const string& current_path) {
    bool changed = false;
    
    if (ImGui::TreeNode("Dependencies")) {
        if (ImGui::Button("Auto Populate")) {
            string contents = get_contents(to_resource_path(current_path).string());

            uint64 start_at = 0;
            while (start_at < contents.size()) {
                uint64 sb_index = contents.find(".sb", start_at);
                if (sb_index == string::npos)
                    break;
                uint64 start_quote = contents.rfind('"', sb_index) + 1;
                uint64 end_quote = contents.find_first_of('"', sb_index);
                dependencies.push_back(string(contents.data() + start_quote, end_quote - start_quote));
                
                start_at = end_quote;
            }
            
        }
        changed |= ImGui::UnorderedVector(dependencies,
            [](string& dep) {
                float width = ImGui::GetContentRegionAvail().x;
                float text_width = ImGui::CalcTextSize("Path").x;
                ImGui::SetNextItemWidth(width - 8.f - text_width);
                return ImGui::PathSelect("Path", &dep, "resources", FileType_General);
            },
            [](vector<string>& deps, bool pressed) {
                if (pressed) {
                    deps.emplace_back();
                }
            },
            true);
        ImGui::TreePop();
    }
    return changed;
}


}

namespace ImGui {
bool PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, spellbook::FileType type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    return PathSelect(hint, out, base_folder, path_filter(type), dnd_key(type), open_subdirectories, context_callback);
}
bool PathSelect(const string& hint, string* out, const string& base_folder, spellbook::FileType type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    return PathSelect(hint, out, base_folder, path_filter(type), dnd_key(type), open_subdirectories, context_callback);
}
}
