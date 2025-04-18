#include "HelloTriangleApplication.h"

// std
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <limits>
#include <algorithm>
#include <unordered_map>


using namespace std;

void HelloTriangleApplication::run()
{
    initWindow();
    m_engine.createInstance(getGLFWRequiredExtensions());
    createWindowSurface(m_engine.getInstance());
    m_engine.setSurface(&m_windowSurface);
    m_engine.init();

    mainLoop();

    m_engine.deinit();
    m_engine.destroyInstance();
    deinitWindow();
}

void HelloTriangleApplication::initWindow()
{
    glfwInit(); // init glfw lib
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // avoid openGL context creation
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // window resize enabled by default

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeChanged);

    glfwGetFramebufferSize(m_window, &m_windowWidth, &m_windowHeight);
}
void HelloTriangleApplication::deinitWindow()
{
    glfwDestroyWindow(m_window);
    glfwTerminate(); // deinit glfw lib
}

void HelloTriangleApplication::createWindowSurface(VkInstance _instance)
{
    if(glfwCreateWindowSurface(_instance, m_window, nullptr, &m_windowSurface) != VkResult::VK_SUCCESS)
        throw::runtime_error("Failed to create the window surface.");
}
void HelloTriangleApplication::destroyWindowSurface(VkInstance _instance)
{
    vkDestroySurfaceKHR(_instance, m_windowSurface, nullptr);
}

vector<const char*> HelloTriangleApplication::getGLFWRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if _DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

void HelloTriangleApplication::mainLoop() 
{
    while (!glfwWindowShouldClose(m_window)) 
    {
        glfwPollEvents();
        m_engine.drawFrame();
    }

    m_engine.idle();
}


void HelloTriangleApplication::framebufferSizeChanged(GLFWwindow* _window, int _width, int _height)
{
    HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(_window));
    app->resizeWindow(_width, _height);
}
void HelloTriangleApplication::resizeWindow(int _width, int _height)
{
    if (_width == 0 || _height == 0)
    {
        glfwGetFramebufferSize(m_window, &_width, &_height);

        while (_width == 0 || _height == 0) 
        {
            glfwGetFramebufferSize(m_window, &_width, &_height);
            glfwWaitEvents();
        }
    }
    m_windowWidth = _width;
    m_windowHeight = _height;
    m_engine.resizeWindow(_width, _height);
}



int main()
{
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

