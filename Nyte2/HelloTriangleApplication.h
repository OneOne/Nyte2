#pragma once

#include <stdint.h>

// class forward decl
class GLFWwindow;

// using
using u32 = uint32_t;

class HelloTriangleApplication 
{
public:
    void run();

private:
    void initWindow();
    void initVulkan();

    void mainLoop();

    void cleanup();

private:
    GLFWwindow* m_window = nullptr;
    static constexpr u32 m_WindowWidth = 1280;
    static constexpr u32 m_WindowHeight = 720;
};
