#include <SDL/include/SDL.h>

namespace dm {

Window::Window(const glm::uvec2& dimensions, const std::string& name)
        : dimensions(dimensions)
        , name(name)
{
    // TODO: Move out when input handling occurs elsewhere?
    DM_ASSERT(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0);
    winHandle = SDL_CreateWindow(
            name.c_str(),
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            dimensions.x,
            dimensions.y,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
    );
    DM_ASSERT(winHandle != nullptr);
}

Window::~Window()
{
    SDL_DestroyWindow(winHandle);
}

}
