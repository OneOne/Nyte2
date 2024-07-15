#include "HelloTriangleApplication.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>

// VCR for Vulkan Check Result
#define VCR(val, msg) if(val != VK_SUCCESS) { throw std::runtime_error(msg); }

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

void HelloTriangleApplication::initVulkan() 
{
    createInstance();
}

void HelloTriangleApplication::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr; // no extension structure
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Retrieve needed extensions for GLFW
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    createInfo.enabledLayerCount = 0; // validation layer count

    // Listing available extension
    {
        u32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "Available extensions:\n";
        for (const VkExtensionProperties& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        // Checking GLFW extensions are available
        bool allGLFWExtensionsAvailable = true;
        for (u32 i = 0; i < glfwExtensionCount; ++i)
        {
            const char* glfwExtStr = glfwExtensions[i];
            auto it = std::find_if(extensions.begin(), extensions.end(),
                [&glfwExtStr](const VkExtensionProperties& extProperties) { return strcmp(extProperties.extensionName, glfwExtStr) == 0; });
            allGLFWExtensionsAvailable &= (it != extensions.end());
        }
        if(!allGLFWExtensionsAvailable)
            std::cout << "One or more GLFW extensions are not supported.\n";
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    VCR(result, "Failed to create instance.");
}

void HelloTriangleApplication::cleanup() 
{
    vkDestroyInstance(m_instance, nullptr);
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