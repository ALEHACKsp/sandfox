# sandfox
## Included Dependencies
* [GLFW](https://github.com/glfw/glfw) v3.3.2
	* OpenGL context, platform window & input devices.
* [Assimp](https://github.com/assimp/assimp) v5.0.1
	* Loading 3D models.
* [Botan](https://github.com/randombit/botan) v2.15.0
	* Various crypto algorithms.
* [CURL](https://github.com/curl/curl) v7.72.0
	* File operations over the internet.
* [spdlog](https://github.com/gabime/spdlog) v1.8.0 (w/ [fmt](https://github.com/fmtlib/fmt) v7.0.3)
	* Logging systems and string formatting.
* [LunaSVG](https://github.com/sammycage/lunasvg) v1.2.0
	* Rendering SVG images.
* [mbed TLS](https://github.com/ARMmbed/mbedtls) v2.24.0
	* Used by CURL for secure algorithms.
* [NNG](https://github.com/nanomsg/nng) v1.3.2
	* Advanced networking schemes.
* [Synapse](https://github.com/zajo/synapse) v0.1.0
	* Same-process signals.
* [UV](https://github.com/libuv/libuv) v1.39.0
	* Async network/filesystem IO & events.
* [Leaf](https://github.com/boostorg/leaf) v0.3.0
	* Error handling.
* [GLM](https://glm.g-truc.net/0.9.9/index.html) v0.9.9.7
	* Mathematics.
* [GLAD](https://github.com/Dav1dde/glad) v0.1.33
	* OpenGL function loading.
* [NanoVG](https://github.com/memononen/nanovg)
	* OpenGL vector graphics & text rendering.
* [STB](https://github.com/nothings/stb)'s ([truetype](https://github.com/nothings/stb/blob/master/stb_truetype.h) v1.24, [image](https://github.com/nothings/stb/blob/master/stb_image.h) v2.10, [image resize](https://github.com/nothings/stb/blob/master/stb_image_resize.h) v0.96)
	* Font rasterization, image loading and sizing.
* [JSON](https://github.com/nlohmann/json) v3.9.1
	* Working with JSON files.

All are CMake subprojects and build automatically aside from Botan which configures itself with a Python script.