#pragma once

#include "general/string.hpp"
#include "general/math/geometry.hpp"
#include "renderer/camera.hpp"
#include "renderer/viewport.hpp"
#include "general/input.hpp"

struct GLFWwindow;

namespace spellbook {

struct CameraController {
    string    name;
    Viewport* viewport = nullptr; // Controller needs a viewport, because input is theoretically passed through the viewport.
    Camera*   camera   = nullptr;

    enum NavigationMode { NavigationMode_None, NavigationMode_Fly };
    NavigationMode nav_mode = NavigationMode_None;

    struct FlyState {
        int   move_state  = 0;
        float speed       = 4.0f;
        float sensitivity = 0.2f;
    } fly_state;

    void change_state(NavigationMode new_mode);
    void setup(Viewport* viewport, Camera* camera);
    void update();
    void cleanup();
};

void inspect(CameraController* controller);

bool cc_on_click(ClickCallbackArgs args);
bool cc_on_cursor(CursorCallbackArgs args);
bool cc_on_scroll(ScrollCallbackArgs args);
bool cc_on_key(KeyCallbackArgs args);

}

