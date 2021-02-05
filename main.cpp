#include "Window.h"
#include "Demos/deferred.h"
#include <sstream>
#include "glfw3.h"


class Application {
public:
    float time = 0.0f;

    Application(const glm::uvec2& windowDimensions)
    {
        time = static_cast<float>(glfwGetTime());
        window = std::shared_ptr<Window>(new Window(windowDimensions, "Vulkan"));
        DeferredRenderingContext::Initialize(window);
    }

    void Run() {
        float dt = 0.0f;
        while (window->Update(dt))
        {
            dt = RenderingContext::dt;
            DeferredRenderingContext::Update();
			DeferredRenderingContext::DrawFrame();
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

