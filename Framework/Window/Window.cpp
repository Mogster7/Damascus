#include "Window.h"
#include "RenderingContext.h"
#include <glfw3.h>

Window::Window(const glm::uvec2& dimensions, const std::string& name) :
    dimensions(dimensions), name(name)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    winHandle = glfwCreateWindow(dimensions.x, dimensions.y, name.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(winHandle);
    glfwSetInputMode(winHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

void Window::SwapBuffer()
{
    glfwSwapBuffers(winHandle);
}
