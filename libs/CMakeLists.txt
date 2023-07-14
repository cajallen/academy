# subdirectories
add_subdirectory(entt)
add_subdirectory(magic_enum)
add_subdirectory(tinygltf)
add_subdirectory(vuk)
add_subdirectory(tracy)
add_subdirectory(vk-bootstrap)


# fetches
include(FetchContent)
FetchContent_Declare(
    glfw 
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG master
)
FetchContent_MakeAvailable(glfw)


# manual libs
add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_glfw.cpp
	imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_link_libraries(imgui PRIVATE glfw)

add_library(lz4 lz4/lz4.cpp lz4/lz4hc.cpp lz4/xxhash.cpp)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE vuk/ext/stb)


# package as interface
add_library(academy_libs INTERFACE)
target_link_libraries(academy_libs INTERFACE 
    magic_enum
    tinygltf
    vuk
    glfw
    lz4
    vk-bootstrap
    imgui
    vk-bootstrap
    robin_hood
    fmt
)
target_include_directories(academy_libs INTERFACE tracy/public)
target_include_directories(academy_libs INTERFACE entt/src)
target_include_directories(academy_libs INTERFACE .)