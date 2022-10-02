//
// Created by dev on 8/27/22.
//
#include "../../kernels/inc/matrix.cuh"
#include "../../kernels/inc/render.cuh"
#include "../../kernels/inc/shader_impl.cuh"
#include "../../util/const.h"
#include "zbuffer.h"
#include "zfiller.h"
#include <helper_math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern __device__ __constant__ mat<4,4> viewport_matrix;

__device__ void triangle_zbuffer(glm::vec3 pts[3], ZBuffer &zbuffer) {
	glm::vec2 bboxmin{float(zbuffer.width-1),  float(zbuffer.height-1)};
	glm::vec2 bboxmax{0., 0.};
	glm::vec2 clamp{float(zbuffer.width-1), float(zbuffer.height-1)};
	for (int i=0; i<3; i++) {
		bboxmin.x = max(0.0f, min(bboxmin.x, pts[i].x));
		bboxmin.y = max(0.0f, min(bboxmin.y, pts[i].y));

		bboxmax.x = min(clamp.x, max(bboxmax.x, pts[i].x));
		bboxmax.y = min(clamp.y, max(bboxmax.y, pts[i].y));
	}


	glm::vec3 P{0, 0, 0};
	int cnt = 0;
	for (P.x=floor(bboxmin.x); P.x<=bboxmax.x; P.x++) {
		for (P.y=floor(bboxmin.y); P.y<=bboxmax.y; P.y++) {
			P.z = 0;
			auto bc_screen  = barycentric(pts, P);
			float bc_screen_idx[3] = {bc_screen.x, bc_screen.y, bc_screen.z};
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
				continue;
			for (int i = 0; i < 3; i++)
				P.z += pts[i].z * bc_screen_idx[i];
			atomicMax(&zbuffer.zbuffer[int(P.x + P.y * zbuffer.width)], P.z);
			cnt++;
			if (cnt > MAX_PIXELS_PER_KERNEL)
				return;
		}
	}
}

__global__ void fill_zbuffer(DrawCallBaseArgs args, ModelArgs model_args, ZBuffer buffer) {
	int position = blockIdx.x * blockDim.x + threadIdx.x;

	auto &model = model_args.model;

	if (position >= model.n_faces)
		return;

	auto face = model.faces[position];
	glm::vec3 world_coords[3];
	glm::vec3 look_dir = args.look_at - args.camera_pos;
	auto sh = Shader(model, {0, 0, 0}, args.projection, args.view, model_args.model_matrix, {1920, 1080});

	for (int i = 0; i < 3; i++)
		sh.vertex(position, i);

	auto &screen_coords = sh.pts;

	for (int j = 0; j < 3; j++)
	{
		glm::vec3 v = model.vertices[at(face, j)];
		world_coords[j] = glm::vec3(v.x, v.y, v.z);
	}

	glm::vec3 n = glm::cross(world_coords[2] - world_coords[0], world_coords[1] - world_coords[0]);
	n = glm::normalize(n);
	float intensity = glm::dot(n, look_dir);
	if (intensity > 0)
		triangle_zbuffer(screen_coords, buffer);
}

__global__ void set_kernel(ZBuffer buffer, float set_to)
{
	const int x = blockIdx.x * blockDim.x + threadIdx.x;
	const int y = blockIdx.y * blockDim.y + threadIdx.y;
	if (x >= buffer.width || y >= buffer.height)
		return;
	buffer.zbuffer[x + y * buffer.width] = set_to; // -FLT_MAX is bad, use cam_z
}

void ZFiller::async_zbuf(DrawCallArgs &args, int model_index) {
	auto &model_args = args.models[model_index];
	auto &model = model_args.model;
	auto n_grid = model.n_faces / 32 + 1;
	auto n_block = 32;
	fill_zbuffer<<<n_grid, n_block, 0, stream>>>(args.base, model_args, zbuffer);
}

ZBuffer ZFiller::get_zbuffer() {
	return ZBuffer{.zbuffer=zbuffer.zbuffer, .width=zbuffer.width, .height=zbuffer.height};
}
void ZFiller::async_reset() {
	const dim3 block(16,16);
	const dim3 grid(divUp(zbuffer.width, block.x), divUp(zbuffer.height, block.y));
	set_kernel<<<grid, block, 0, stream>>>(zbuffer, -FLT_MAX); // fixme set with something else!!!!!!!!!!!!!!!!!!!!!!!!!!
}

void ZFiller::resize(int height, int width) {
	zbuffer.create(height, width);
}
void ZFiller::resize(Image &image) {
	resize(image.height, image.width);
}
