#pragma once
#include "../../ImNBT/include/ImNBT/NBTReader.hpp"
#include "../../ImNBT/include/ImNBT/NBTWriter.hpp"
#include <SDL_video.h>
struct SDL_Window;
union SDL_Event;

namespace dm
{
class Window
{
public:
    Window(const glm::uvec2& dimensions, const std::string& name, bool fullscreen, const glm::uvec2& position = glm::uvec2{ SDL_WINDOWPOS_UNDEFINED });

    ~Window();

    [[nodiscard]] inline SDL_Window* GetHandle() const
    {
        return winHandle;
    }

    [[nodiscard]] inline glm::uvec2 GetDimensions() const
    {
        return dimensions;
    }

    [[nodiscard]] inline glm::uvec2 GetPosition() const
    {
        int x, y;
#if defined(OS_Windows) || defined(OS_Linux)
        SDL_GetWindowPosition(winHandle, &x, &y);
#else
        assert(false && "Desktop Support only");
#endif
        return { x, y };
    }

    [[nodiscard]] inline double GetDPIScale() const
    {
        return dpiScale;
    }

    void DeserializeWindow(ImNBT::Reader&& reader);

    // if the file exists, set mergeExisting to true so we can merge data
    void SerializeWindow(std::string_view path, bool mergeExisting);

    void ToggleFullscreen();

    void ToggleMaximize();

private:
    glm::uvec2 dimensions = {};
    const std::string name = {};
    SDL_Window* winHandle = {};
    double dpiScale = 1.0;
    bool isFullscreenInternal = false;
    bool isMaximized = false;
};


} // namespace dm