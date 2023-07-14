#pragma once

#include "extension/fmt.hpp"
#include "general/vector.hpp"
#include "game/game_scene.hpp"

namespace spellbook {

struct EditorScene {
    string name;
    std::unique_ptr<GameScene> scene = {};
    
    EditorScene() { scene = std::make_unique<GameScene>(); }

    virtual void setup() { scene->name = fmt_("{}::GameScene", name); scene->setup(); }
    virtual void update() { scene->update(); }
    virtual void window(bool* p_open) { }
    virtual void shutdown() { scene->shutdown(); }

    virtual void settings_window(bool* open);
    virtual void output_window(bool* open);
};


inline vector<EditorScene*>& get_editor_scenes() {
    static vector<EditorScene*> vec;
    return vec;
}

template<typename T>
struct AddEditorScene {
    AddEditorScene(T* t, const string& name) {
        get_editor_scenes().push_back(new T());
        get_editor_scenes().back()->name = name;
    }
};

#define ADD_EDITOR_SCENE(T) \
    AddEditorScene _Add##T((T*) 0, #T);
    

}