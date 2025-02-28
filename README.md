# sehle
An OpenGL graphics library written in C

## Overview
Sehle is an object-oriented graphics library on top of OpenGL, built in C.
- Uses AZ type system
- A bit lower level than scene graph/game libraries
- Designed as drop-in static library

## Building
You need arikkei, az (for type system) and elea (for 3D math) libraries.
The simplest way is to clone [arikkei](https://github.com/lauris71/arikkei), [az](https://github.com/lauris71/az) and [elea](https://github.com/lauris71/elea) to the root of sehle directory.

    git clone git@github.com:lauris71/sehle.git
    cd sehle
    git clone git@github.com:lauris71/arikkei.git
    git clone git@github.com:lauris71/az.git
    git clone git@github.com:lauris71/elea.git

Then, in the same sehle project directory execute:

    cmake -S . -B build
    cmake --build build

The provided makefiles build static library.

For the idea how it works check out the tutorials subdirectory.

## Design principles
Sehle is meant as an intermediate layer between the raw graphics API (OpenGL) and higher-level library (scene graph, game engine). Thus most sehle datatypes are not very convenient to manage directly and should be embedded in higher-level objects instead.

### Engine
This is an all-round manager of the underlying rendering system

### Resources
These are just wrappers around OpenGL resources
- textures
- programs
- vertex and index buffers
- render targets

Resources are AZObjects - i.e. self-contained, reference-counted types. In addition the engine manages a lookup and cache system to avoid resource replication.

### Renderables
Renderables are types that can schedule rendering calls. All visible things, including lights, are derived from abstract SehleRenderable superclass.

SehleRenderable is an interface type. Thus the memory management and implementation has to be provided by user code. Normally one should implement a renderable interface(s) inside a high-level graphical object.

The library provides few convenience objects that wrap SehleRenderable interfaces:
- Lights
- StaticMesh
- RenderableList

These types are bare AZ block types and thus need external memory mamagement.

### Materials
Materials manage binding of programs to renderables during render calls.

SehleMaterial is an interface type. Thus the memory management and implementation has to be provided by user code. 

The library provdes some convenience material objects that implement material interfaces:
- Lights
- DNS (Diffuse/Normal/Specular) material
- ControlMaterial - a simple single-texture forward-rendered material

These types are bare AZ block types and thus need external memory mamagement.

### Render context
Render context holds temporary low and high-level data about scene, such as:
- OpenGL state
- render target(s)
- render lists
- camera matrixes

It is bare AZ block type with pretty lightweight constructors and destructors, thus readily buildable in stack.

### Renderers
Renderers are high-level mini-engines that perform specific complex rendering operations:
- Scene renderer
- MSAA renderer
- UI renderer
- Sky renderer
