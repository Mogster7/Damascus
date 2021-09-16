#pragma once
struct SDL_Window;
union SDL_Event;

namespace dm {
class Window
{
public:
	Window(const glm::uvec2& dimensions, const std::string& name);

	~Window();

	[[nodiscard]] inline SDL_Window* GetHandle() const
	{
		return winHandle;
	}

	[[nodiscard]] inline glm::uvec2 GetDimensions() const
	{
		return dimensions;
	}

    [[nodiscard]] inline double GetDPIScale() const
    {
        return dpiScale;
    }

private:
	glm::uvec2 dimensions = {};
	const std::string name = {};
    SDL_Window* winHandle = {};
    double dpiScale = 1.0;
};


}