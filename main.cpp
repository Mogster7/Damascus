#include "Window.h"
#include "RenderingContext.h"
#include <sstream>
#include "glfw3.h"
#include "ECS/ECS.h"


class Application {
public:
    RenderingContext& renderingContext;

    Application(const glm::uvec2& windowDimensions, RenderingContext& renderingContext)
        : renderingContext(renderingContext)
    {
        ECS::CreateSystems();
        window = std::shared_ptr<Window>(new Window(windowDimensions, "Vulkan"));
        renderingContext.Initialize(window);
    }

    void Run() {
        float dt = 0.0f;
        while (window->Update(dt))
        {
            ECS::UpdateSystems(dt);
            window->SwapBuffer();
            renderingContext.Update();
            dt = renderingContext.dt;
			renderingContext.Draw();
        }
        
		renderingContext.Destroy();
        ECS::DestroySystems();
    }

private:
    std::shared_ptr<Window> window;

};

int main() {
    Application app({ 1600,900 }, RenderingContext::Get());

    try {
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	return EXIT_SUCCESS;
}

