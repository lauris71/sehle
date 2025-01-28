# sehle
An OpenGL graphics library written in C

## Overview
This an object-oriented graphics library on top of OpenGL, built in C.
- It uses AZ RTTI and reflection system
- A bit lower level than scene graph/game libraries
- Designed as drop-in static library

## Building
You need arikkei, az (for RTT) and elea (for 3D math) libraries. Just clone [arikkei](https://github.com/lauris71/arikkei), [az](https://github.com/lauris71/az) and [elea](https://github.com/lauris71/elea) to an accessible place (either to the root of the main project or elea).
Then, in the az project directory execute:

    cmake -S . -B build
    cmake --build build

The provided makefiles builds static library.

## Design principles
It is meant as an intermediate layer between the raw graphics API (OpenGL) and higher-level graphical library (scene graph, game engine). Due to that the most sehle datatypes are not very convenient to manage and should be embedded in higher-level objects instead.

### Engine
This is an all-round manager of the overall system

### Resources
These are mostly wrappers around OpenGL resources
- textures
- programs
- vertex and index buffers
- render targets

These are implemented as AZObjects, i.e. self-contained, reference-counted types. In addition, the engine manages a lookup system for discovering resources to avoid replication.

### Renderables
Types that schedule rendering calls. All visible things, including lights, are derived from abstract SehleRenderable.

SehleRenderable is an AZInterface subclass. Thus the memory management and implementation has to be provided by user code. Normally one should implement a renderable interface(s) inside a high-level graphical object.

The library provides few convenience objects that wrap SehleRenderable interfaces:
- Lights
- StaticMesh

These are bare AZ block types and thus need external memory mamagement.

### Materials
Materials manage binding of programs to renderables during render calls.

SehleMaterial is an AZInterface subclass. Thus the memory management and implementation has to be provided by user code. 

The library provdes some convenience material objects that implement material interfaces:
- Lights
- DNS (Diffuse/Normal/Specular) material
- ControlMaterial - a simple single-texture forward-rendered material

These are bare AZ block types and thus need external memory mamagement.

### Render context
This holds temporary and high-level data about scene, such as:
- render state
- render target(s)
- render lists
- camera materixes

### Renderers
These are high-level mini-engines that perform specific complex rendering operations:
- scene renderer
- MSAA renderer
- UI renderer
- Sky renderer
