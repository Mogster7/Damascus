#pragma once
struct GLFWwindow;

class Window
{
public:
    Window(const glm::uvec2& dimensions, const std::string& name);
    ~Window();

    // bool returns whether or not window should close
    bool Update(float dt);

    GLFWwindow* GetHandle() const { return winHandle; }
    inline glm::uvec2 GetDimensions() const { return dimensions; }

    
private:
    glm::uvec2 dimensions = {};
    const std::string name = "";

    GLFWwindow* winHandle = {};
};


