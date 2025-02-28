//
// Created by dev on 7/13/22.
//

#ifndef COURSE_RENDERER_TEXTURE_CUH
#define COURSE_RENDERER_TEXTURE_CUH

#include <vector_types.h>
#include <string>

struct TextureRef
{
	int x = 0;
	int y = 0;
	int n = 0;
	uchar3 *data{};
	[[nodiscard]] __device__ uchar3 get_uv(float u, float v) const;
};

struct Texture
{
	int x = 0;
	int y = 0;
	int n = 0;
	uchar3 *data{};

	Texture() = default;
	~Texture();
	explicit Texture(const std::string &filename);
	static Texture get_default();

	[[nodiscard]] TextureRef get_ref() const;
};

#endif//COURSE_RENDERER_TEXTURE_CUH
