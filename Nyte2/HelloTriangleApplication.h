#pragma once

#include <stdint.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

// class forward decl
struct GLFWwindow;

// using
using u32 = uint32_t;

class HelloTriangleApplication 
{
public:
    void run();

private:
    void initWindow();
    void initVulkan();

    void createInstance();

    std::vector<const char*> getRequiredExtensions();

#if _DEBUG
    bool checkValidationLayerSupport();
    void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
    void setupDebugMessenger();
#endif

    void cleanup();

    void mainLoop();

private:
    GLFWwindow* m_window = nullptr;
    static constexpr u32 m_WindowWidth = 1280;
    static constexpr u32 m_WindowHeight = 720;

    VkInstance m_instance;

#if _DEBUG
    VkDebugUtilsMessengerEXT m_callback;
#endif
};
