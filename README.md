![damascus_gray_shadowed](https://user-images.githubusercontent.com/26748509/127756150-da6d3d30-f764-4277-aa00-7bf0a0111080.png)


Damascus interface using a unity build with CMake

### Features

* Vulkan class abstractions with automatic memory management
* Descriptor set management with automatic host-to-device uploading
* Shader reflection 

### Files of note

Demonstration of simple type abstraction: `Framework/InternalStructures/Semaphore.h`

Type definitions for abstraction: `Framework/RenderingDefines.hpp`

Main descriptor set management logic: `Framework/InternalStructures/Descriptors.h/cpp`

Renderer (rendering context management): `Framework/Renderer/Renderer.h/cpp`


