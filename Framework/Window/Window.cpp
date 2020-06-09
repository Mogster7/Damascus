#include "Window.h"
#include "RenderingStructures.hpp"
#include "Renderer.h"
#include <glfw3.h>

Window::Window(const glm::uvec2& dimensions, const std::string& name) :
    m_Dimensions(dimensions), m_Name(name)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_WinHandle = glfwCreateWindow(dimensions.x, dimensions.y, name.c_str(), nullptr, nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(m_WinHandle);
    glfwTerminate();
}


bool Window::Update(float dt)
{
    if (glfwWindowShouldClose(m_WinHandle))
        return false;
        
    
    glfwPollEvents();



    return true;
}
