//
// Created by dev on 8/28/22.
//
#include "scene.h"
#include <imgui.h>

std::shared_ptr<Camera> Scene::get_camera() {
	return camera;
}
std::vector<ModelArgs> Scene::get_models() {
	auto model_args = std::vector<ModelArgs>(models.size());
	for (int i = 0; i < models.size(); i++) {
		model_args[i] = models[i].to_args();
	}
	return model_args;
}

DrawCallArgs Scene::get_draw_call_args() {
	DrawCallArgs args = {
			.models = get_models(),
			.base = {
					.light_dir = light_dir,
					.camera_pos = camera->position,
					.look_at = camera->position + camera->get_look_direction(),
					.view_matrix = camera->get_view_matrix(),
					.projection_matrix = camera->get_projection_matrix(),
			},
	};
	return args;
}
float3 &Scene::get_light_dir() {
	return light_dir;
}
void Scene::set_camera(const Camera &cam) {
	camera = std::make_shared<Camera>(cam);
}


void Scene::display_menu() {
	ImGui::Begin("Scene Controls");
	ImGui::SliderFloat3("Light dir", &light_dir.x, -1, 1);
	ImGui::End();

	camera->display_menu();

	ImGui::Begin("Models");
	for (int i = 0; i < models.size(); i++)
	{
		if (ImGui::TreeNode((void*)(intptr_t)i, "Model %d", i))
		{
			ImGui::SliderFloat3("Position", &models[i].position.x, -10, 10);
			ImGui::TreePop();
		}
	}
	ImGui::End();
}
void Scene::add_model(const StoredModel &model) {
	models.push_back(model);
}
StoredModel &Scene::get_model(int index) {
	return models[index];
}

int  Scene::get_n_models() {
	return models.size();
}