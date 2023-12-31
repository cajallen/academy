#include "camera_controller.hpp"

#include <imgui.h>

#include "extension/glfw.hpp"
#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/math/math.hpp"
#include "general/math/matrix_math.hpp"

namespace spellbook {

bool cc_on_mouse(ClickCallbackArgs args) {
    CameraController& cc = *((CameraController*) args.data);

    if (args.button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (args.action != GLFW_RELEASE && cc.nav_mode != CameraController::NavigationMode_Fly)
            cc.change_state(CameraController::NavigationMode_Fly);
        else
            cc.change_state(CameraController::NavigationMode_None);
    }

    return false;
}

bool cc_on_cursor(CursorCallbackArgs args) {
    CameraController& cc = *((CameraController*) args.data);

    if (cc.nav_mode == CameraController::NavigationMode_Fly) {
        // negative yaw is "right", think of circle degrees from origin's perspective
        cc.camera->heading.yaw -= Input::mouse_delta.x * Input::delta_time * cc.fly_state.sensitivity;
        cc.camera->heading.pitch -= Input::mouse_delta.y * Input::delta_time * cc.fly_state.sensitivity;
        cc.camera->heading.pitch = math::clamp(cc.camera->heading.pitch, -math::PI / 2 + 0.01f, math::PI / 2 - 0.01f);

        cc.camera->view_dirty = true;
        return true;
    }

    return false;
}

bool cc_on_scroll(ScrollCallbackArgs args) {
    CameraController& cc = *((CameraController*) args.data);

    if (!cc.viewport->hovered)
        return false;

    if (cc.nav_mode == CameraController::NavigationMode_Fly) {
        if (Input::shift) {
            cc.fly_state.speed *= math::pow(1.1, args.yoffset);
            return true;
        }

        if (Input::alt) {
            cc.camera->fov -= 2.5 * args.yoffset;
            cc.camera->proj_dirty = true;
            return true;
        }
    }

    return false;
}

bool cc_on_key(KeyCallbackArgs args) {
    CameraController& cc = *((CameraController*) args.data);
    if (!cc.viewport->hovered && !cc.viewport->focused && cc.nav_mode != CameraController::NavigationMode_Fly)
        return false;

    if (args.key == GLFW_KEY_F && args.action == GLFW_PRESS) {
        auto new_mode = cc.nav_mode == CameraController::NavigationMode_Fly ? CameraController::NavigationMode_None
                                                                            : CameraController::NavigationMode_Fly;
        cc.change_state(new_mode);
        return true;
    }
    return false;
}

void CameraController::change_state(NavigationMode new_mode) {
    switch (nav_mode) {
        case (NavigationMode_None): {
        } break;
        case (NavigationMode_Fly): {
            Input::set_cursor_disabled(false);
        } break;
    }
    nav_mode = new_mode;
    switch (nav_mode) {
        case (NavigationMode_None): {
        } break;
        case (NavigationMode_Fly): {
            Input::set_cursor_disabled(true);
        } break;
    }
}

void CameraController::update() {
    v3 world_velocity = v3(0.0f);
    v3 local_velocity = v3(0.0f);

    if (ImGui::GetIO().WantCaptureKeyboard || !(viewport->hovered || viewport->focused))
        return;

    if (nav_mode == NavigationMode_Fly) {
        if (Input::key_down[GLFW_KEY_A])
            local_velocity -= v3(1, 0, 0);
        if (Input::key_down[GLFW_KEY_D])
            local_velocity += v3(1, 0, 0);
        if (Input::key_down[GLFW_KEY_W]) // Unfortunately for depth precision reasons, -Z is forward.
            local_velocity -= v3(0, 0, 1);
        if (Input::key_down[GLFW_KEY_S])
            local_velocity += v3(0, 0, 1);
        if (Input::key_down[GLFW_KEY_Q])
            local_velocity += v3(0, 1, 0);
        if (Input::key_down[GLFW_KEY_E])
            local_velocity -= v3(0, 1, 0);

        if (math::length_squared(local_velocity) >= 0.5f) {
            m33 basis      = (m33) math::transpose(camera->view);
            world_velocity = basis * local_velocity;
            world_velocity = math::normalize(world_velocity) * fly_state.speed;
            camera->position += v3(world_velocity * Input::delta_time);
            camera->view_dirty = true;
        }
    }

}

void inspect(CameraController* controller) {
    switch (controller->nav_mode) {
        case (CameraController::NavigationMode_None): {
            ImGui::Text("Navigation Mode: None");
        } break;
        case (CameraController::NavigationMode_Fly): {
            ImGui::Text("Navigation Mode: Flying");
        } break;
    }
    if (ImGui::TreeNode("Fly State")) {
        ImGui::DragFloat("Speed", &controller->fly_state.speed, 0.01f);
        ImGui::DragFloat("Sensitivity", &controller->fly_state.sensitivity, 0.01f);

        ImGui::Text("Move State: ");
        if (controller->fly_state.move_state & Direction_Left) {
            ImGui::SameLine();
            ImGui::Text("LEFT ");
        }
        if (controller->fly_state.move_state & Direction_Right) {
            ImGui::SameLine();
            ImGui::Text("RIGHT ");
        }
        if (controller->fly_state.move_state & Direction_Forward) {
            ImGui::SameLine();
            ImGui::Text("FORWARD ");
        }
        if (controller->fly_state.move_state & Direction_Backward) {
            ImGui::SameLine();
            ImGui::Text("BACKWARD ");
        }
        if (controller->fly_state.move_state & Direction_Up) {
            ImGui::SameLine();
            ImGui::Text("UP ");
        }
        if (controller->fly_state.move_state & Direction_Down) {
            ImGui::SameLine();
            ImGui::Text("DOWN ");
        }
        ImGui::TreePop();
    }
}

void CameraController::setup(Viewport* init_viewport, Camera* init_camera) {
    viewport = init_viewport;
    camera   = init_camera;
    Input::add_callback(InputCallbackInfo{cc_on_mouse, 20, name, this});
    Input::add_callback(InputCallbackInfo{cc_on_cursor, 20, name, this});
    Input::add_callback(InputCallbackInfo{cc_on_scroll, 20, name, this});
    Input::add_callback(InputCallbackInfo{cc_on_key, 20, name, this});
}

void CameraController::cleanup() {
    Input::remove_callback<ClickCallback>(name);
    Input::remove_callback<CursorCallback>(name);
    Input::remove_callback<ScrollCallback>(name);
    Input::remove_callback<KeyCallback>(name);
}

}
