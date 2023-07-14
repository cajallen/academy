#pragma once

#include "general/vector.hpp"
#include "game/game_scene.hpp"

namespace spellbook {

struct EditorScene {
    string name;
    GameScene* scene = nullptr;
    
    virtual void setup() { scene->setup(); }
    virtual void update() { scene->update(); }
    virtual void window(bool* p_open) { }
    virtual void shutdown() { scene->shutdown(); }

    virtual void settings_window(bool* open) {}
    virtual void output_window(bool* open) {}
};


struct EditorScenes {
    template<typename T>
    EditorScenes(T* t, const string& name) {
        values().push_back(new T());
        values().back()->name = name;
    }

    static vector<EditorScene*>& values() {
        static vector<EditorScene*> vec;
        return vec;
    }
};
#define ADD_EDITOR_SCENE(T, name) \
    EditorScenes _Add##T((T*) 0, name);
    

}