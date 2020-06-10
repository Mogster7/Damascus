#include "Window.h"
#include "Renderer.h"
#include <sstream>


class Application {
public:
    Application(const glm::uvec2& windowDimensions)
    {
        window = std::make_shared<Window>(windowDimensions, "Vulkan");
        Renderer::Initialize(window);
        Renderer::MakeMesh();
    }

    void Run() {
        while (window->Update(0.0f))
        {
            Renderer::DrawFrame();
        }
    }

private:
    std::shared_ptr<Window> window;

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

