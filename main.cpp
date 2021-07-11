#include "Window.h"
#include "RenderingContext.h"
#include <sstream>
#include "glfw3.h"
#include "ECS/ECS.h"



class Application {
public:

    Application(const glm::uvec2& windowDimensions)
    	: window(std::make_shared<Window>(windowDimensions, "Vulkan"))
    	, renderingContext(&RenderingContext::Get())
    {
        ECS::CreateSystems();
        JobSystem::Initialize();
        renderingContext->Create(window);
    }

    void Run() {
        float dt = 0.0f;


        while (window->Update(dt))
        {

            ECS::UpdateSystems(dt);

            window->SwapBuffer();
            renderingContext->Update();
            dt = renderingContext->dt;
			renderingContext->Draw();
        }

        JobSystem::Destroy();
		renderingContext->Destroy();
        ECS::DestroySystems();
    }

private:
    std::shared_ptr<Window> window;
	bk::RenderingContext* renderingContext;

};

int main() {
    Application app({ 1600,900 });

    app.Run();

	return EXIT_SUCCESS;
}

