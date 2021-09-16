#include <SDL/include/SDL.h>
#ifdef OS_Linux
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#endif

#include <SDL_syswm.h>

namespace dm {

constexpr double baseDPI = 96.0;

#ifdef OS_Linux
double PlatformGetMonitorDPI(SDL_Window* win)
{
    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);
    SDL_GetWindowWMInfo(win, &sysInfo);

    char *resourceString = XResourceManagerString(sysInfo.info.x11.display);
    XrmDatabase db;
    XrmValue value;
    char *type = NULL;
    double dpi = 0.0;

    XrmInitialize(); /* Need to initialize the DB before calling Xrm* functions */

    db = XrmGetStringDatabase(resourceString);

    if (resourceString) {
        printf("Entire DB:\n%s\n", resourceString);
        if (XrmGetResource(db, "Xft.dpi", "String", &type, &value) == True) {
            if (value.addr) {
                dpi = atof(value.addr);
            }
        }
    }

    printf("DPI: %f\n", dpi);
    return dpi;
}
#elif OS_Windows
// TODO: Determine DPI scaling
double PlatformGetMonitorDPI(SDL_Window* win)
{
    return baseDPI;
}
#endif

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
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    dpiScale = PlatformGetMonitorDPI(winHandle) / baseDPI;
    SDL_SetWindowSize(winHandle, dimensions.x * dpiScale, dimensions.y * dpiScale);
    DM_ASSERT(winHandle != nullptr);
}

Window::~Window()
{
    SDL_DestroyWindow(winHandle);
}

}
