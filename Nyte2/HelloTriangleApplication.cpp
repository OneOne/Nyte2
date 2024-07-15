#include "HelloTriangleApplication.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow()
{
    glfwInit(); // init glfw lib
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // avoid openGL context creation
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable window resize (for now)

    m_window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, "Vulkan", nullptr, nullptr);
}
void HelloTriangleApplication::initVulkan() {

}

void HelloTriangleApplication::cleanup() 
{
    glfwDestroyWindow(m_window);

    glfwTerminate(); // deinit glfw lib
}

void HelloTriangleApplication::mainLoop() 
{
    while (!glfwWindowShouldClose(m_window)) 
    {
        glfwPollEvents();
    }
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