//
// Created by dev on 10/4/22.
//

#include "scene_loader.h"
#include "../model/inc/pool.h"
#include "scene.h"

#include <imgui/imgui.h>

void SceneLoader::display_widget() {

	if (ImGui::CollapsingHeader("Load Scene"))
	{
		if (!can_load_scene())
		{
			ImGui::Text("To load scene:\n 1. Disable virtual geometry");
			return;
		}
		for (auto &callback : this->load_callbacks) {
			if (ImGui::Button(callback.first.c_str())) {
				SceneSingleton().get()->clear();
				ModelPoolCreator().get()->clear();
				callback.second();
			}
		}
	}
}
void SceneLoader::register_load_scene(const std::string &name, const std::function<void()> &callback) {
	load_callbacks[name] = callback;
}
void SceneLoader::load_scene(const std::string &name) {
	load_callbacks[name]();
}
void SceneLoader::register_can_load_scene(const std::function<bool()> &callback) {
	can_load_scene = callback;
}
