# cgtb
Experiments with a "minimal redraw" user interface to avoid wasting computer resources on nothing.

## Building
### Ubuntu
#### Generate source files for embedded resources
- ```cd src/app/emico/```
- ```python3 emico.py```
#### Install OpenGL development files
- ```apt install libgl-dev```
#### Install GLFW3 dependencies
- ```apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev```
#### Clone repository & submodules
- ```git clone https://github.com/codegoose/cgtb.git --recursive```
#### Run CMake
- ```mkdir cgtb/ubuntu```
- ```cd cgtb/ubuntu/```
- ```cmake --build .```
