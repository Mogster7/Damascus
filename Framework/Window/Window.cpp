#include "Window.h"
#include "Renderer.h"
#include <glfw3.h>

Window::Window(const glm::uvec2& dimensions, const std::string& name) :
    dimensions(dimensions), name(name)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    winHandle = glfwCreateWindow(dimensions.x, dimensions.y, name.c_str(), nullptr, nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(winHandle);
    glfwTerminate();
}


bool Window::Update(float dt)
{
    if (glfwWindowShouldClose(winHandle))
        return false;
        
    
    glfwPollEvents();



    return true;
}
