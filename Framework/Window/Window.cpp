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
  // TODO: Move out when SDL handles input and such
  SDL_Quit();
}


bool Window::Update(float dt)
{
	if (!(winHandle))
	{
		return false;
	}
	SDL_Event event;

  while(SDL_PollEvent(&event))
  {
    for(auto& eventProcessor : eventProcessors)
    {
      eventProcessor(&event);
    }

    switch (event.type)
    {
      case SDL_MOUSEBUTTONDOWN:
        std::cout << "You pressed the mouse, you win a cashew from Jon" << std::endl;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
          case SDLK_ESCAPE:
            return false;
        }
      case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
          case SDL_WINDOWEVENT_CLOSE:
            return false;
        }
        break;
      case SDL_QUIT:
        return false;
        break;
    }
  }
	return true;
}

void Window::SwapBuffer()
{
}

}