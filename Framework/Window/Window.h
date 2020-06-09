#pragma once
struct GLFWwindow;

class Window
{
public:
    Window(const glm::uvec2& dimensions, const std::string& name);
    ~Window();

    // bool returns whether or not window should close
    bool Update(float dt);

    GLFWwindow* GetHandle() const { return m_WinHandle; }
    inline glm::uvec2 GetDimensions() const { return m_Dimensions; }

    
private:
    glm::uvec2 m_Dimensions = {};
    const std::string m_Name = "";

    GLFWwindow* m_WinHandle = {};
};


