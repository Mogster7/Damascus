#pragma once
struct SDL_Window;
union SDL_Event;

namespace dm {
class Window
{
public:
	Window(const glm::uvec2& dimensions, const std::string& name);

	~Window();

	// bool returns whether or not window should close
	bool Update(float dt);

	void SwapBuffer();

	[[nodiscard]] inline SDL_Window* GetHandle() const
	{
		return winHandle;
	}

	[[nodiscard]] inline glm::uvec2 GetDimensions() const
	{
		return dimensions;
	}

  std::vector<std::function<bool(SDL_Event*)>> eventProcessors;

private:
	glm::uvec2 dimensions = {};
	const std::string name = {};
  SDL_Window* winHandle = {};
};


}