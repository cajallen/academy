#include "game_scene.hpp"

#include "renderer/renderer.hpp"

namespace spellbook {

void GameScene::setup() {
	camera = Camera(v3(-8.0f, 8.0f, 6.0f), math::d2r(euler{-45.0f, -30.0f}));
	render_scene.viewport.name	 = render_scene.name + "::viewport";
	render_scene.viewport.camera = &camera;
	render_scene.viewport.setup();

	get_renderer().add_scene(&render_scene);
}

void GameScene::update() {

}

void GameScene::shutdown() {
	render_scene.cleanup(*get_renderer().global_allocator);
}

}