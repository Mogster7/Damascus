#include "Window.h"
#include "Renderer.h"
#include <sstream>


class Application {
public:
    Application(const glm::uvec2& windowDimensions)
    {
        window = eastl::shared_ptr<Window>(new Window(windowDimensions, "Vulkan"));
        Renderer::Initialize(window);
    }

    void Run() {
        float dt = 0.0f;
        while (window->Update(dt))
        {
            Renderer::Update();
            dt = Renderer::dt;
            Renderer::DrawFrame();
        }
    }

private:
    eastl::shared_ptr<Window> window;

};

int main() {
    Application app({ 800, 600 });


    try {
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
	return EXIT_SUCCESS;
}

