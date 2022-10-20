//
// Created by dev on 10/8/22.
//

#include "virtual_model.h"
#include "../../../util/const.h"
#include <helper_cuda.h>

ModelDrawCallArgs VirtualModel::to_args() {
	assert(scene_object_id.has_value());
	await();

	return {
		.model = vmodel,
		.disabled_faces = disabled_faces,
	};
}

VirtualModel::VirtualModel() {
	vmodel.n_faces = 0;
	vmodel.n_vertices = 0;
}

VirtualModel::~VirtualModel() {
	free();
}

void VirtualModel::accept(ModelDrawCallArgs args, bool *disabled_faces_to_copy) {
	assert(!scene_object_id.has_value());

	scene_object_id = args.scene_object_id;
	geometry_upsampler = std::make_unique<GeometryUpsampler>(vmodel, stream);

	int max_texture_index = args.model.max_texture_index;

	if (n_bad_faces > vmodel.n_faces)
	{
		cudaFree(vmodel.faces);
		cudaFree(vmodel.textures_for_face);
		checkCudaErrors(cudaMalloc((void **) (&vmodel.faces), sizeof(glm::ivec3) * n_bad_faces));
		checkCudaErrors(cudaMalloc((void **) (&vmodel.textures_for_face), sizeof(glm::ivec3) * n_bad_faces));
		vmodel.n_faces = n_bad_faces;
	}
	if (n_bad_vertices > vmodel.n_vertices)
	{
		checkCudaErrors(cudaMalloc((void **) (&vmodel.vertices), sizeof(glm::vec3) * n_bad_vertices));
		checkCudaErrors(cudaMalloc((void **) (&vmodel.normals), sizeof(glm::vec3) * n_bad_vertices));
		vmodel.n_vertices = n_bad_vertices;
	}
	if (max_texture_index > vmodel.max_texture_index)
	{
		checkCudaErrors(cudaMalloc((void **) (&vmodel.textures), sizeof(glm::vec2) * max_texture_index));
		vmodel.max_texture_index = max_texture_index;
	}
	if (args.model.n_faces > m_allocated_disabled_faces)
	{
		checkCudaErrors(cudaMalloc((void **) (&disabled_faces), sizeof(glm::ivec3) * args.model.n_faces));
		m_allocated_disabled_faces = args.model.n_faces;
	}
	update_virtual_model(args, disabled_faces_to_copy);
}

void VirtualModel::free() {
	cudaFree(vmodel.faces);
	cudaFree(vmodel.vertices);
	cudaFree(vmodel.normals);
	cudaFree(vmodel.textures);
	cudaFree(vmodel.textures_for_face);
	cudaFree(disabled_faces);
}

void VirtualModel::release() {
	assert(scene_object_id.has_value());
	scene_object_id = std::nullopt;
	last_updated = std::chrono::system_clock::now();
}

int VirtualModel::get_model_id() {
	assert(scene_object_id.has_value());
	return vmodel.id;
}

void VirtualModel::update_virtual_model(ModelDrawCallArgs original_model, bool *disabled_faces_to_copy)
{
	cudaMemcpyAsync(disabled_faces, disabled_faces_to_copy, sizeof(bool) * m_allocated_disabled_faces, cudaMemcpyDeviceToDevice);
	geometry_upsampler->async_upsample_geometry(original_model, disabled_faces);
	last_updated = std::chrono::system_clock::now();
}

void VirtualModel::update(ModelDrawCallArgs model, bool *disabled_faces_to_copy) {
	using namespace std::chrono_literals;
	assert(scene_object_id.has_value());
	assert(scene_object_id.value() == model.scene_object_id);
	auto since_update = (std::chrono::system_clock::now() - last_updated) / 1ms;
	if (since_update < UPDATE_VIRTUAL_MODEL_EVERY_MS)
		return;
	update_virtual_model(model, disabled_faces_to_copy);
}

bool *VirtualModel::get_disabled_faces() {
	await();
	return disabled_faces;
}
