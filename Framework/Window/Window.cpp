#include <SDL/include/SDL.h>
#ifdef OS_Linux
    #include <X11/Xlib.h>
    #include <X11/Xresource.h>
#endif

#include "Window.h"
#include <SDL_syswm.h>


namespace dm
{

constexpr double baseDPI = 96.0;

#ifdef OS_Linux
double PlatformGetMonitorDPI(SDL_Window* win)
{
    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);
    SDL_GetWindowWMInfo(win, &sysInfo);

    char* resourceString = XResourceManagerString(sysInfo.info.x11.display);
    XrmDatabase db;
    XrmValue value;
    char* type = NULL;
    double dpi = 0.0;

    XrmInitialize(); /* Need to initialize the DB before calling Xrm* functions */

    db = XrmGetStringDatabase(resourceString);

    if (resourceString)
    {
        printf("Entire DB:\n%s\n", resourceString);
        if (XrmGetResource(db, "Xft.dpi", "String", &type, &value) == True)
        {
            if (value.addr)
            {
                dpi = atof(value.addr);
            }
        }
    }

    printf("DPI: %f\n", dpi);
    return dpi;
}
#else
double PlatformGetMonitorDPI(SDL_Window* win)
{
    float dpi;
    SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(win), &dpi, NULL, NULL);

    return dpi;
}
#endif

Window::Window(const glm::uvec2& dimensions, const std::string& name, bool fullscreen, const glm::uvec2& position)
    : dimensions(dimensions), name(name), isFullscreenInternal(fullscreen)
{

    // TODO: Move out when input handling occurs elsewhere?
    DM_ASSERT(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == 0);

    winHandle = SDL_CreateWindow(
            name.c_str(),
            position.x,
            position.y,
            dimensions.x,
            dimensions.y,
            SDL_WINDOW_VULKAN
    );
    SDL_SetWindowBordered(winHandle, SDL_TRUE);
    DM_ASSERT_MSG(winHandle != nullptr, SDL_GetError());

#ifdef OS_Mac
    dpiScale = 1.0;
#else
    dpiScale =  PlatformGetMonitorDPI(winHandle) / baseDPI;
#endif

    // In case of a zero DPI return
    if (dpiScale == 0)
        dpiScale = 1;

    // SDL_SetWindowPosition(winHandle, 0, 0);

    if(isFullscreenInternal)
    {
        SDL_SetWindowFullscreen(winHandle, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
        SDL_SetWindowSize(winHandle, dimensions.x, dimensions.y);
    }


}

Window::~Window()
{
    SDL_DestroyWindow(winHandle);
}

void Window::ToggleFullscreen()
{
    isFullscreenInternal = !isFullscreenInternal;
    if(isFullscreenInternal)
    {
        SDL_SetWindowFullscreen(winHandle, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
        SDL_SetWindowFullscreen(winHandle, 0);
        SDL_SetWindowSize(winHandle, dimensions.x, dimensions.y);
    }
}

void Window::ToggleMaximize()
{
    isMaximized = !isMaximized;
    if (isMaximized)
    {
        SDL_MaximizeWindow(winHandle);
    }
    else
    {
        SDL_RestoreWindow(winHandle);
    }
}

void Window::DeserializeWindow(ImNBT::Reader&& reader)
{

    glm::uvec2 position{ SDL_WINDOWPOS_UNDEFINED };
#if defined(OS_Windows) || defined(OS_Linux)

    auto isActiveDisplay = [](const SDL_Rect& displayBounds) {
        for (int i = 0; i < SDL_GetNumVideoDisplays(); i++)
        {
            auto windowsEqual = [](const SDL_Rect& w1, const SDL_Rect& w2) {
                return w1.x == w2.x && w1.y == w2.y && w1.h == w2.h && w1.w == w2.w;
            };

            SDL_Rect otherBounds;
            SDL_GetDisplayBounds(i, &otherBounds);
            if (windowsEqual(displayBounds, otherBounds))
            {
                return true;
            }
        }
        return false;
    };

    if (reader.OpenList("Windows"))
    {
        for (int i = 0; i < reader.ListSize(); ++i)
        {
            if (reader.OpenCompound(""))
            {
                if (reader.OpenCompound("DisplayBounds"))
                {
                    SDL_Rect otherBounds;
                    otherBounds.x = reader.ReadInt("x");
                    otherBounds.y = reader.ReadInt("y");
                    otherBounds.w = reader.ReadInt("w");
                    otherBounds.h = reader.ReadInt("h");
                    reader.CloseCompound();

                    if (isActiveDisplay(otherBounds))
                    {
                        if (reader.OpenCompound("Position"))
                        {
                            position.x = reader.ReadInt("x");
                            position.y = reader.ReadInt("y");
                            reader.CloseCompound();
                            i = std::numeric_limits<int>::max() - 1; // set i to exit condition to break out early and properly close compounds/lists
                        }
                    }
                }
                reader.CloseCompound();
            }
        }
        reader.CloseList();
    }

    SDL_SetWindowPosition(winHandle, position.x, position.y);

#endif
}

void Window::SerializeWindow(std::string_view path, bool mergeExisting)
{
#if defined(OS_Windows) || defined(OS_Linux)
    glm::uvec2 position = GetPosition();
    glm::uvec2 halfSize = GetDimensions();
    halfSize.x /= 2;
    halfSize.y /= 2;
    glm::uvec2 windowCenter = position + halfSize;
    SDL_Rect displayBounds;
    // find the display bounds of whichever display this is in the center of
    for (int i = 0; i < SDL_GetNumVideoDisplays(); i++)
    {
        SDL_GetDisplayBounds(i, &displayBounds);
        // check if window center is inside the display
        if (windowCenter.x >= displayBounds.x && windowCenter.y >= displayBounds.y)
        {
            if (windowCenter.x <= displayBounds.x + displayBounds.w && windowCenter.y <= displayBounds.y + displayBounds.h)
            {
                break;
            }
        }
    }

    auto windowsEqual = [](const SDL_Rect& w1, const SDL_Rect& w2) {
        return w1.x == w2.x && w1.y == w2.y && w1.h == w2.h && w1.w == w2.w;
    };

    std::vector<std::pair<SDL_Rect, glm::uvec2>> monitors;
    monitors.push_back({ displayBounds, position });

    if (mergeExisting)
    {
        auto reader = ImNBT::Reader(path);
        if (reader.OpenList("Windows"))
        {
            for (int i = 0; i < reader.ListSize(); ++i)
            {
                if (reader.OpenCompound(""))
                {

                    if (reader.OpenCompound("DisplayBounds"))
                    {
                        SDL_Rect otherBounds;
                        otherBounds.x = reader.ReadInt("x");
                        otherBounds.y = reader.ReadInt("y");
                        otherBounds.w = reader.ReadInt("w");
                        otherBounds.h = reader.ReadInt("h");
                        reader.CloseCompound();
                        if (!windowsEqual(otherBounds, displayBounds))
                        {
                            monitors.emplace_back();
                            monitors.back().first = otherBounds;
                            if (reader.OpenCompound("Position"))
                            {
                                monitors.back().second.x = reader.ReadInt("x");
                                monitors.back().second.y = reader.ReadInt("y");
                                reader.CloseCompound();
                            }
                        }
                    }
                }
                reader.CloseCompound();
            }
            reader.CloseList();
        }
    }

    auto writer = ImNBT::Writer(path);
    if (writer.BeginList("Windows"))
    {
        for (const auto& monitor : monitors)
        {
            if (writer.BeginCompound(""))
            {
                if (writer.BeginCompound("DisplayBounds"))
                {
                    writer.WriteInt(monitor.first.x, "x");
                    writer.WriteInt(monitor.first.y, "y");
                    writer.WriteInt(monitor.first.w, "w");
                    writer.WriteInt(monitor.first.h, "h");
                    writer.EndCompound();
                }
                if (writer.BeginCompound("Position"))
                {
                    writer.WriteInt(monitor.second.x, "x");
                    writer.WriteInt(monitor.second.y, "y");
                    writer.EndCompound();
                }
            }
            writer.EndCompound();
        }
        writer.EndList();
    }

#endif
}

} // namespace dm
