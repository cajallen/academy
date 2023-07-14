#pragma once

#include "general/vector.hpp"
#include "game/game_scene.hpp"

namespace spellbook {

struct EditorScene {
    string name;
    std::unique_ptr<GameScene> scene = {};
    
    EditorScene() { scene = std::make_unique<GameScene>(); }

    virtual void setup() { scene->setup(); }
    virtual void update() { scene->update(); }
    virtual void window(bool* p_open) { }
    virtual void shutdown() { scene->shutdown(); }

    virtual void settings_window(bool* open);
    virtual void output_window(bool* open);
};


struct EditorScenes {
    static vector<EditorScene*>& values() {
        static vector<EditorScene*> vec;
        return vec;
    }
};

template<typename T>
struct AddEditorScene {
    AddEditorScene(T* t, const string& name) {
        EditorScenes::values().push_back(new T());
        EditorScenes::values().back()->name = name;
    }
};

#define ADD_EDITOR_SCENE(T) \
    AddEditorScene _Add##T((T*) 0, #T);
    

}