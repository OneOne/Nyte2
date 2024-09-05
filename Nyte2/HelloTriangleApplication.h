#pragma once

// stl
#include <optional>
#include <chrono>

// glfw
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Nyte/Engine.h"

// class forward decl
struct GLFWwindow;



class HelloTriangleApplication 
{
public:
    void run();

private:
    void initWindow();
    void deinitWindow();
    
    void createWindowSurface(VkInstance _instance);
    void destroyWindowSurface(VkInstance _instance);

    std::vector<const char*> getGLFWRequiredExtensions();
    void mainLoop();

    static void framebufferSizeChanged(GLFWwindow* window, int width, int height);
    void resizeWindow(int _width, int _height);

private:
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 1280;
    int m_windowHeight = 720;

    Nyte::Engine m_engine;
    VkSurfaceKHR m_windowSurface;

    

};
