#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define ENET_IMPLEMENTATION
#include <enet.h>
#include <glad.h>
#include <nanovg.h>
#define NANOVG_GLES2_IMPLEMENTATION
#include <nanovg_gl.h>

NVGcontext *sf_impl_nvg_create() {
	return nvgCreateGLES2(NVG_ANTIALIAS);
}

void sf_impl_nvg_destroy(NVGcontext *ctx) {
	nvgDeleteGLES2(ctx);
}