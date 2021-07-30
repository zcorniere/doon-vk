# doon-vk

## Description

I created this projet with the goal of experenting with the Vulkan library and to teach myself how this library work. This is nothing more that an PoC, at least for now.

### Feature

#### Partial Forward Renderer

The CPU make as few command call to Vulkan as possible, allowing the GPU to compute much faster. I am aiming at making the engine as bindless as possible.

To achieve that, I am using a array of texture, and each object has its texture id. So there is only one pipeline, and one descriptor set for the entire render pass.

All the vertices and indices are also loaded into one single buffer, so I only have to bind one vertex buffer and one index buffer for the entire render pass.

#### Basic light

The Phong lighting algorithm is implemented, provding basic ambient, diffuse and specular light. A material system is also implemented to change the light reflected by the objects.

#### `.obj` file loading

It is capable of opening .obj file, and loading the verticies, the normals and the UV coordinates. All of witch are then send to the graphics card for the rendering.

## How To

First, you need to have Vulkan installed, with the dev headers.

then

```bash
git clone --recursive git@github.com:zcorniere/doon-vk.git
cd doon-vk
mkdir build && cd build
cmake .. && make
```
