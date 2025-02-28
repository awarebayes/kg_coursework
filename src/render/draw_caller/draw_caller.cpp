//
// Created by dev on 8/27/22.
//

#include "draw_caller.h"
#include "../../util/stream_manager.h"
#include <future>
#include <helper_functions.h>
#include <helper_math.h>


void DrawCaller::draw(DrawCallArgs args_unculled, Image &image)
{
	// interface
	interface->log_fps();
	interface->start();
	interface->log_before_culling(args_unculled.models.size());

	DrawCallArgs args;

	if (interface->is_culling_enabled()) {
		args = culler->cull(args_unculled, *args_unculled.base.camera_ptr);
	} else {
		args = args_unculled;
	}
	interface->log_after_culling(args.models.size());

	if (interface->is_virtual_geometry_enabled()) {
		virtual_geometry_manager->populate_virtual_models(args, image, args_unculled);
	}

	// dispatch
	image_resetter->async_reset(image);
	size_t streams_to_use = std::min(args.models.size(), (size_t)n_streams);
	for (size_t i = 0; i < streams_to_use; i++)
	{
		zfillers[i]->resize(image);
		zfillers[i]->async_reset();
	}

	for (size_t i = 0; i < args.models.size(); i++)
		zfillers[i % zfillers.size()]->async_zbuf(args, i);

	// sync
	for (size_t i = 0; i < streams_to_use; i++)
		zfillers[i]->await();
	image_resetter->await();

	auto zbuffers = get_z_buffers();
	// parallel dispatch and sync
	parallel_reduce_merge(z_mergers, zbuffers, (int)streams_to_use);
	auto final_zbuffer = get_final_z_buffer();

	// dispatch
	for (size_t i = 0; i < args.models.size(); i++)
		rasterizers[i % rasterizers.size()]->async_rasterize(args, i, image, final_zbuffer);

	// sync
	for (size_t i = 0; i < streams_to_use; i++)
		rasterizers[i]->await();

	interface->draw_widget();
	interface->stop();
}

DrawCaller::DrawCaller() {
	image_resetter = std::make_shared<ImageResetter>();
	culler = std::make_shared<Culler>();
	interface = std::make_shared<RenderInterface>();
	virtual_geometry_manager = std::make_shared<VirtualGeometryManager>();
	for (int i = 0; i < n_streams; i++)
	{
		rasterizers.emplace_back(std::make_shared<Rasterizer>());
		zfillers.emplace_back(std::make_shared<ZFiller>());
		z_mergers.emplace_back(std::make_shared<ZMerger>());
	}
	interface->register_vgeometry_manager(*virtual_geometry_manager);
}

std::vector<ZBuffer> DrawCaller::get_z_buffers() {
	std::vector<ZBuffer> zbuffers{zfillers.size()};
	for (size_t i = 0; i < zfillers.size(); i++)
		zbuffers[i] = zfillers[i]->get_zbuffer();
	return zbuffers;
}
ZBuffer DrawCaller::get_final_z_buffer() {
	return zfillers[0]->get_zbuffer();
}
bool DrawCaller::can_load_scene() {
	return !interface->is_virtual_geometry_enabled();
}
