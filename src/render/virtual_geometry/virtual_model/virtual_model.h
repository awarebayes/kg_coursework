//
// Created by dev on 10/8/22.
//

#ifndef COURSE_RENDERER_VIRTUAL_MODEL_H
#define COURSE_RENDERER_VIRTUAL_MODEL_H

#include "../../misc/draw_caller_args.cuh"
#include "../geometry_upsampler/geometry_upsampler.h"
#include <optional>

const int UPDATE_VIRTUAL_MODEL_EVERY_MS = 100;

class VirtualModel : public Synchronizable
{
private:
	std::unique_ptr<GeometryUpsampler> geometry_upsampler{};
	std::optional<int> scene_object_id = std::nullopt;
	std::chrono::time_point<std::chrono::system_clock> last_updated{};
	ModelRef vmodel;
	bool *disabled_faces = nullptr;
	size_t m_allocated_disabled_faces = 0;
	size_t n_bad_faces = 1000;
	size_t n_bad_vertices = 1000;
	void free();
	void update_virtual_model(ModelDrawCallArgs original_model, bool *disabled_faces_to_copy);

public:
	VirtualModel();
	~VirtualModel() override;

	void accept(ModelDrawCallArgs model, bool *disabled_faces_to_copy);
	void update(ModelDrawCallArgs model, bool *disabled_faces_to_copy);
	void release();
	int get_model_id();
	bool *get_disabled_faces();
	bool holds_nothing() { return !scene_object_id.has_value(); };
	ModelDrawCallArgs to_args();
};

#endif//COURSE_RENDERER_VIRTUAL_MODEL_H
