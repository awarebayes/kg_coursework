#include "../../model/inc/model.h"
#include "../../model/inc/pool.h"
#include "../../util/stream_manager.h"
#include "../inc/matrix.cuh"
#include "../inc/render.cuh"
#include "../inc/shader_impl.cuh"
#include "../inc/util.cuh"
#include <ctime>
#include <helper_cuda.h>
#include <thrust/fill.h>


__device__ __constant__ glm::mat4 viewport_matrix{};
__device__ glm::mat4 projection_matrix{};
__device__ glm::mat4 view_matrix{};


__device__ void line(Image &image, int x0, int y0, int x1, int y1) {
	bool steep = false;
	if (std::abs(x0-x1)<std::abs(y0-y1)) {
		swap(x0, y0);
		swap(x1, y1);
		steep = true;
	}
	if (x0>x1) {
		swap(x0, x1);
		swap(y0, y1);
	}
	__syncthreads();

	uint color = rgbaFloatToInt(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});

	for (int x=x0; x<=x1; x++) {
		float t = (x-x0)/(float)(x1-x0);
		int y = y0*(1.-t) + y1*t;
		int x_draw = y * steep + x * (1 - steep);
		int y_draw = x * steep + y * (1 - steep);
		image.set(x_draw, y_draw, color);
	}
}


template <typename Tp>
__device__ __forceinline__ glm::vec3 barycentric(glm::vec3 *pts, Tp P) {
	auto a = glm::vec3{float(pts[2].x-pts[0].x), float(pts[1].x-pts[0].x), float(pts[0].x-P.x)};
	auto b = glm::vec3{float(pts[2].y-pts[0].y), float(pts[1].y-pts[0].y), float(pts[0].y-P.y)};
	auto u = cross(a, b);
	float flag = abs(u.z) < 1;
	return glm::vec3{
	                -1.0f * flag + (1.0f - flag) * (1.f-(u.x+u.y)/u.z),
	                 1.0f * flag + (1.0f - flag) * (u.y/u.z),
	                 1.0f * flag + (1.0f - flag) * (u.x/u.z)
	};
}


__device__ void triangle_zbuffer(glm::vec3 pts[3], Image &image) {
	float2 bboxmin{float(image.width-1),  float(image.height-1)};
	float2 bboxmax{0., 0.};
	float2 clamp{float(image.width-1), float(image.height-1)};
	for (int i=0; i<3; i++) {
		bboxmin.x = max(0.0f, min(bboxmin.x, pts[i].x));
		bboxmin.y = max(0.0f, min(bboxmin.y, pts[i].y));

		bboxmax.x = min(clamp.x, max(bboxmax.x, pts[i].x));
		bboxmax.y = min(clamp.y, max(bboxmax.y, pts[i].y));
	}

	glm::vec3 P{0, 0, 0};
	for (P.x=floor(bboxmin.x); P.x<=bboxmax.x; P.x++) {
		for (P.y=floor(bboxmin.y); P.y<=bboxmax.y; P.y++) {
			P.z = 0;
			auto bc_screen  = barycentric(pts, P);
			float bc_screen_idx[3] = {bc_screen.x, bc_screen.y, bc_screen.z};
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
				continue;
			for (int i = 0; i < 3; i++)
				P.z += pts[i].z * bc_screen_idx[i];
			atomicMax(&image.zbuffer[int(P.x + P.y * image.width)], P.z);
		}
	}
}


__device__ void triangle(DrawCallArgs &args, int position, Image &image) {
	auto &model = args.model;
	auto light_dir = args.light_dir;


	glm::mat4 transform_mat = viewport_matrix * projection_matrix * args.model_matrix * view_matrix;

	auto sh = Shader(model, light_dir);
	sh.transform_M = transform_mat;

	for (int i = 0; i < 3; i++)
		sh.vertex(position, i);

	auto &pts = sh.pts;
	auto &normals = sh.normals;
	auto &textures = sh.textures;

	if (pts[0].y==pts[1].y && pts[0].y==pts[2].y) return;

	float2 bboxmin{float(image.width-1),  float(image.height-1)};
	float2 bboxmax{0., 0.};
	float2 clamp{float(image.width-1), float(image.height-1)};
	for (auto &pt : pts) {
		bboxmin.x = max(0.0f, min(bboxmin.x, pt.x));
		bboxmin.y = max(0.0f, min(bboxmin.y, pt.y));

		bboxmax.x = min(clamp.x, max(bboxmax.x, pt.x));
		bboxmax.y = min(clamp.y, max(bboxmax.y, pt.y));
	}

	glm::vec3 P{0, 0, 0};

	for (P.x=floor(bboxmin.x); P.x <= bboxmax.x; P.x++) {
		for (P.y=floor(bboxmin.y); P.y <= bboxmax.y; P.y++) {
			auto bc_screen  = barycentric(pts, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
				continue;

			P.z = 0;
			for (int i = 0; i < 3; i++)
				P.z += pts[i].z * at(bc_screen, i);

			if (image.zbuffer[int(P.x + P.y* image.width)] == P.z) {
				uint color;
				sh.fragment(bc_screen, color);
				image.set((int)P.x, (int)P.y, color);
			}
		}
	}
}

__global__ void fill_zbuffer(DrawCallArgs args) {
	auto &model = args.model;
	auto &image = args.image;
	int position = blockIdx.x * blockDim.x + threadIdx.x;

	if (position >= model.n_faces)
		return;
	auto face = model.faces[position];
	glm::vec3 screen_coords[3];
	glm::vec3 world_coords[3];
	glm::vec3 look_dir = args.look_at - args.camera_pos;

	glm::mat4 transform_mat = viewport_matrix * projection_matrix * args.model_matrix * view_matrix;

	for (int j = 0; j < 3; j++)
	{
		glm::vec3 v = model.vertices[at(face, j)];
		screen_coords[j] = transform_mat * glm::vec4(v.x, v.y, v.z, 1.0f);
		world_coords[j] = v;
	}

	glm::vec3 n = cross(world_coords[2] - world_coords[0], world_coords[1] - world_coords[0]);
	n = normalize(n);
	float intensity = dot(n, look_dir);
	if (intensity > 0)
		triangle_zbuffer(screen_coords, image);
}


__global__ void draw_faces(DrawCallArgs args) {
	auto &model = args.model;
	auto &image = args.image;

	int position = blockIdx.x * blockDim.x + threadIdx.x;
	if (position >= model.n_faces)
		return;
	auto face = model.faces[position];
	glm::vec3 world_coords[3];
	auto look_dir = args.look_at - args.camera_pos;
	for (int j = 0; j < 3; j++)
	{
		glm::vec3 v = model.vertices[at(face, j)];
		world_coords[j] = v;
	}

	glm::vec3 n = cross(world_coords[2] - world_coords[0], world_coords[1] - world_coords[0]);
	n = normalize(n);
	float intensity = dot(n, look_dir);
	if (intensity > 0)
		triangle(args, position, image);
}


void render_init(int width, int height)
{
	int depth = 255;
	glm::mat4 ViewPort = viewport(width/8, height/8, width*3/4, height*3/4, depth);
	cudaMemcpyToSymbol(
	        viewport_matrix,
	        &ViewPort,
	        sizeof(glm::mat4)
	        );
}

void update_device_parameters(const DrawCallArgs &args)
{
	glm::mat4 Projection = glm::mat4(1.0f);

	Projection[3][2] = -1.f / args.camera_pos.z;
	cudaMemcpyToSymbol(
	        projection_matrix,
	        &Projection,
	        sizeof(glm::mat4)
	        );

	glm::mat4 View = lookat(args.camera_pos, args.look_at, {0, 1, 0});
	cudaMemcpyToSymbol(
        view_matrix,
        &View,
        sizeof(glm::mat4)
        );
}

double main_cuda_launch(const DrawCallArgs &args, StopWatchInterface *timer) {
	auto streams = SingletonCreator<StreamManager>().get();

	// var for kernel computation timing
	// sync host and start kernel computation timer
	double dKernelTime;
	// sync host and start kernel computation timer
	dKernelTime = 0.0;

	clock_t begin = clock();
	sdkResetTimer(&timer);

	update_device_parameters(args);
	streams->prepare_to_render();

	auto &model = args.model;
	auto &image = args.image;
	int n_grid = model.n_faces / 32 + 1;
	int n_block = 32;

	cudaMemsetAsync((void *)args.image.pixels, 0, args.image.width * args.image.height * sizeof(uint), streams->render);
	fill_zbuffer<<<n_grid, n_block, 0, streams->render>>>(args);
	draw_faces<<<n_grid, n_block, 0, streams->render>>>(args);

	checkCudaErrors(cudaStreamSynchronize(streams->render));
	thrust::fill(thrust::device, image.zbuffer, image.zbuffer + image.width * image.height, -FLT_MAX);
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

	printf("%f ms\n", elapsed_secs * 1000);
	dKernelTime = sdkGetTimerValue(&timer);

	return dKernelTime / 1000.;
}